/**
   Copyright (c) 2013 eQualit.ie under Gnu AGPL V3 or later. For more 
   information please see http://www.gnu.org/licenses/agpl.html

   This is a module which talk to ATS and retrieve the information the filters
   needed. Very often different filter needs similar informations. This is
   to reduce transaction and reduce code size

   You should only use one Muncher per request and destroy it after 
   the transaction is done. This goes with ATS "continuation" mentality.
   
   Vmon: Sept 2013: Initial version
 */

#include <map>
#include <string.h>

#include <ts/ts.h>
//to retrieve the client ip
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "transaction_muncher.h"

using namespace std;
/**
   checks if the parts are already retrieved and if not ask ATS 
   to fullfill the part. Through exception if it fails

   @param a set of flags of TransactionPart type indicate what part of 
   GET transaction is needed
              
   @return A map of transaction parts
*/
const TransactionParts&
TransactionMuncher::retrieve_parts(uint64_t requested_log_parts)
{
  //only turn those bits that are off in valid_parts and off in requested
  uint64_t parts_to_retreive = (~valid_parts) & requested_log_parts;

  //IP
  if (parts_to_retreive & TransactionMuncher::IP) {
      //Retrieving the ip address
      //so we assume IPv4 for now
      struct sockaddr_in* client_address = (struct sockaddr_in*) TSHttpTxnClientAddrGet(trans_txnp);
    
      if (!client_address) {
        TSError("error in retrieving client ip\n");
        throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
      }

      cur_trans_parts[TransactionMuncher::IP] = inet_ntoa(client_address->sin_addr); //TODO who should release the char* returned?
      //TSfree(client_address);
  }

  if (parts_to_retreive & TransactionMuncher::URL) {
    TSMLoc url_loc;
    if (TSHttpHdrUrlGet(request_header, header_location, &url_loc) != TS_SUCCESS) {
      TSError("couldn't retrieve request url\n");
      TSHandleMLocRelease(request_header, header_location, url_loc);
      throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
    }

    int url_length;
    const char* url = TSUrlStringGet(request_header, url_loc, &url_length);
      
    if (!url){
      TSError("couldn't retrieve request url string\n");
      TSHandleMLocRelease(request_header, header_location, url_loc);
      throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
    }

    //I'm not sure if we need to release URL explicitly
    //my guess is not
    cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::URL, string(url,url_length)));
    
    TSHandleMLocRelease(request_header, header_location, url_loc);
    //TSfree(url);

  } 
  
  if (parts_to_retreive & TransactionMuncher::HOST) {
    TSMLoc host_loc = TSMimeHdrFieldFind(request_header, header_location, TS_MIME_FIELD_HOST, TS_MIME_LEN_HOST);

    if (host_loc == TS_NULL_MLOC) {
      TSError("couldn't retrieve request host\n");
      //We are not throwing exception cause a request may not have host
      //though in http 1.1 host is required.
      //hence in this case host will be empty
    } else {
      int host_length;
      const char* host = TSMimeHdrFieldValueStringGet(request_header,header_location,host_loc,0,&host_length);
      if (!host) {
        TSHandleMLocRelease(request_header, header_location, host_loc);
        TSError("couldn't retrieve request host string\n");
      } else {
        cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::HOST, string(host,host_length)));

        TSHandleMLocRelease(request_header, header_location, host_loc);
        //TSfree(host);
      }
    }
    
  } 
  
  if (parts_to_retreive & TransactionMuncher::UA) {
    TSMLoc ua_loc = TSMimeHdrFieldFind(request_header, header_location, TS_MIME_FIELD_USER_AGENT, TS_MIME_LEN_USER_AGENT);
    if (ua_loc == TS_NULL_MLOC)  {
      TSError("couldn't retrieve request user-agent\n");
    } else {
      int ua_length;
      const char* ua = TSMimeHdrFieldValueStringGet(request_header, header_location,ua_loc,0,&ua_length);	
      if (!ua) {                                                                                 TSHandleMLocRelease(request_header, header_location, ua_loc);
        TSError("couldn't retrieve request user-agent string\n");
      } else {
        cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::UA, string(ua,ua_length)));
        
        TSHandleMLocRelease(request_header, header_location, ua_loc);
        //TSfree((void*)ua);
      }
      
    }

  } 
  
  if (parts_to_retreive & TransactionMuncher::COOKIE) {
    TSMLoc cookie_loc = TSMimeHdrFieldFind(request_header, header_location, TS_MIME_FIELD_COOKIE, TS_MIME_LEN_COOKIE);

    if (cookie_loc != TS_NULL_MLOC) {
      int cookie_length;
      const char* cookie_value = TSMimeHdrFieldValueStringGet(request_header, header_location,cookie_loc,0,&cookie_length);
      if (cookie_value) {
        cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::COOKIE, string(cookie_value,cookie_length)));
        //TSfree(cookie_value);
      }

      TSHandleMLocRelease(request_header, header_location, cookie_loc);

    }

  }

  valid_parts |= requested_log_parts;

  return cur_trans_parts;

}

void
TransactionMuncher::set_status(TSHttpStatus status)
{
  TSMLoc hdr_loc;
  TSMBuffer bufp;

  if (TSHttpTxnClientRespGet(trans_txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
    TSError("couldn't retrieve client response header\n");
    TSHttpTxnReenable(trans_txnp, TS_EVENT_HTTP_CONTINUE);
    return;
  }

  TSHttpHdrStatusSet(bufp, hdr_loc, status);
  TSHttpHdrReasonSet(bufp, hdr_loc,
                       TSHttpHdrReasonLookup(status),
                       strlen(TSHttpHdrReasonLookup(status)));

}

TransactionMuncher::TransactionMuncher(TSHttpTxn cur_txnp)
  :valid_parts(0), request_header(NULL), header_location(NULL)
{
  trans_txnp = cur_txnp;

  //we retrieve the header anyway
  if (TSHttpTxnClientReqGet(trans_txnp, &request_header, &header_location) != TS_SUCCESS) {
    TSError("couldn't retrieve client response header\n");
    throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
  }

}

/**
   destructor releasing the header buffer
 */
TransactionMuncher::~TransactionMuncher()
{
  TSDebug("banjax", "holy shit!");
  TSHandleMLocRelease(request_header, TS_NULL_MLOC, header_location);
}
