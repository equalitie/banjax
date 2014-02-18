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

#include <assert.h>

#include <ts/ts.h>
//to retrieve the client ip
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "banjax_common.h" 
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

  //if we have nothing to do let save time
  if (!parts_to_retreive)
    return cur_trans_parts;

  //we retrieve the header anyway
  if (!request_header)
    if (TSHttpTxnClientReqGet(trans_txnp, &request_header, &header_location) != TS_SUCCESS) {
      TSError("couldn't retrieve client response header\n");
      throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
    }

  //IP
  if (parts_to_retreive & TransactionMuncher::IP) {
      //Retrieving the ip address
      //so we assume IPv4 for now
      struct sockaddr_in* client_address = (struct sockaddr_in*) TSHttpTxnClientAddrGet(trans_txnp);
    
      if (!client_address) {
        TSError("error in retrieving client ip\n");
        TSDebug(BANJAX_PLUGIN_NAME, "scray stuff: ip-less transation");
        cur_trans_parts[TransactionMuncher::IP] = "";
        //throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
      }
      else {

        cur_trans_parts[TransactionMuncher::IP] = inet_ntoa(client_address->sin_addr); //TODO who should release the char* returned?
      //TSfree(client_address);
      }
  }

  //it is just worth it to retrieve the scheme at the same time
  if (parts_to_retreive & TransactionMuncher::URL || parts_to_retreive & TransactionMuncher::PROTOCOL) {
    TSMLoc url_loc;
    if (TSHttpHdrUrlGet(request_header, header_location, &url_loc) != TS_SUCCESS) {
      TSError("couldn't retrieve request url\n");
      TSHandleMLocRelease(request_header, header_location, url_loc);
      //throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
      cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::URL, ""));
      cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::PROTOCOL, ""));
    }
    else {
      int url_length;
      const char* url = TSUrlStringGet(request_header, url_loc, &url_length);
    
      if (!url){
	TSError("couldn't retrieve request url string\n");
	TSHandleMLocRelease(request_header, header_location, url_loc);
	cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::URL, ""));
	cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::PROTOCOL, ""));

	//throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
      }
      else {

	//I'm not sure if we need to release URL explicitly
	//my guess is not
	cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::URL, string(url,url_length)));

	int protocol_length;
	const char* protocol = TSUrlSchemeGet(request_header, url_loc , &protocol_length);
	cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::PROTOCOL, string(protocol,protocol_length)));
    
	TSHandleMLocRelease(request_header, header_location, url_loc);
      }
    } 
  }

  if (parts_to_retreive & TransactionMuncher::URL_WITH_HOST) {
    //first make sure HOST has already been retrieved 
    retrieve_parts(TransactionMuncher::HOST | TransactionMuncher::URL);

    TSMLoc url_loc;
    if (TSHttpHdrUrlGet(request_header, header_location, &url_loc) != TS_SUCCESS) {
      TSError("couldn't retrieve request url\n");
      TSHandleMLocRelease(request_header, header_location, url_loc);
      //throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
      cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::URL_WITH_HOST, ""));
    } else {

      if (TSUrlHostSet(request_header, url_loc, cur_trans_parts[TransactionMuncher::HOST].c_str(), cur_trans_parts[TransactionMuncher::HOST].length()) != TS_SUCCESS) {
	TSError("couldn't manipulate url field.\n");
	TSHandleMLocRelease(request_header, header_location, url_loc);
	//throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
	
      }
      else {
	int url_length;
	const char* url = TSUrlStringGet(request_header, url_loc, &url_length);
      
	if (!url){
	  TSError("couldn't retrieve request url string\n");
	  TSHandleMLocRelease(request_header, header_location, url_loc);
	  cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::URL_WITH_HOST, ""));
	  //throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
	}
	else {
	  //I'm not sure if we need to release URL explicitly
	  //my guess is not
	  cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::URL_WITH_HOST, string(url,url_length)));
	  TSDebug(BANJAX_PLUGIN_NAME, "resp url %s", cur_trans_parts[TransactionMuncher::URL_WITH_HOST].c_str());
    
	  TSHandleMLocRelease(request_header, header_location, url_loc);
	  //TSfree(url);
	}
      }
    }
  } 

  //METHOD
  if (parts_to_retreive & TransactionMuncher::METHOD) {
    int method_length;
    const char* http_method = TSHttpHdrMethodGet(request_header, header_location, &method_length);

    if (!http_method){
      TSError("couldn't retrieve request method\n");
      cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::METHOD, ""));
      //throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
    }
    else {
      //I'm not sure if we need to release URL explicitly
      //my guess is not
      cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::METHOD, string(http_method,method_length)));
    }
  } 
   
  if (parts_to_retreive & TransactionMuncher::HOST) {
    TSMLoc host_loc = TSMimeHdrFieldFind(request_header, header_location, TS_MIME_FIELD_HOST, TS_MIME_LEN_HOST);

    if (host_loc == TS_NULL_MLOC) {
      TSError("couldn't retrieve request host\n");
      //We are not throwing exception cause a request may not have host
      //though in http 1.1 host is required.
      //hence in this case host will be empty
      cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::HOST, ""));
    } else {
      int host_length;
      const char* host = TSMimeHdrFieldValueStringGet(request_header,header_location,host_loc,0,&host_length);
      if (!host) {
        TSHandleMLocRelease(request_header, header_location, host_loc);
        TSError("couldn't retrieve request host string\n");
	cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::HOST, ""));
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
      cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::UA, ""));
    } else {
      int ua_length;
      const char* ua = TSMimeHdrFieldValueStringGet(request_header, header_location,ua_loc,0,&ua_length);	
      if (!ua) {                                                                                 TSHandleMLocRelease(request_header, header_location, ua_loc);
        TSError("couldn't retrieve request user-agent string\n");
	cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::UA, ""));
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
      } else {
        cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::COOKIE, ""));
      }

      TSHandleMLocRelease(request_header, header_location, cookie_loc);

    } else {
      cur_trans_parts.insert(pair<uint64_t, string> (TransactionMuncher::COOKIE, ""));
    }

  }


  valid_parts |= requested_log_parts;
  update_validity_status();

  return cur_trans_parts;

}

const TransactionParts&
TransactionMuncher::retrieve_response_parts(uint64_t requested_log_parts)
{
  //only turn those bits that are off in valid_parts and off in requested
  uint64_t parts_to_retreive = (~valid_parts) & requested_log_parts;
  
  //if we have nothing to do let save time
  if (!parts_to_retreive)
    return cur_trans_parts;

  //First check if we need to retrieve the header
  if (!response_header) retrieve_response_header();

  //I need the status to take other actions anyway
  if (parts_to_retreive & TransactionMuncher::STATUS) {
    TSHttpStatus resp_status;

    resp_status = TSHttpHdrStatusGet(response_header, response_header_location);

    cur_trans_parts[STATUS] = to_string(resp_status);
    valid_parts |= STATUS;
  }

 if (parts_to_retreive & TransactionMuncher::CONTENT_LENGTH) {
  //if (TS_HTTP_STATUS_OK == resp_status) {

    TSMLoc field_loc = TSMimeHdrFieldFind(response_header, response_header_location, TS_MIME_FIELD_CONTENT_LENGTH, TS_MIME_LEN_CONTENT_LENGTH);
    if (!field_loc) {
      cur_trans_parts[CONTENT_LENGTH] = "0";
    }
    else {
      int field_length;
      const char* content_length = TSMimeHdrFieldValueStringGet(response_header, response_header_location, field_loc, 0, &field_length);
      
      cur_trans_parts[CONTENT_LENGTH] = string(content_length, field_length);

      //}
        
    }
    valid_parts |= CONTENT_LENGTH;
 }
 
 update_validity_status();
 return cur_trans_parts;

}

void 
TransactionMuncher::retrieve_response_header()
{
  assert(trans_txnp);
  if (TSHttpTxnClientRespGet(trans_txnp, &response_header, &response_header_location) != TS_SUCCESS) {
    TSError("couldn't retrieve client response header\n");
    throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
  }

}

void
TransactionMuncher::set_status(TSHttpStatus status)
{
  //First check if we need to retrieve the header
  if (!response_header) 
    retrieve_response_header();

  TSHttpHdrStatusSet(response_header, response_header_location, status);
  TSHttpHdrReasonSet(response_header, response_header_location,
                       TSHttpHdrReasonLookup(status),
                       strlen(TSHttpHdrReasonLookup(status)));

}

void
TransactionMuncher::append_header(const std::string& name, const std::string& value)
{
  if (!response_header) 
    retrieve_response_header();
  
  TSMLoc field_location = NULL;

  if ( TSMimeHdrFieldCreate(response_header, response_header_location, &field_location) == TS_SUCCESS ) {
    TSMimeHdrFieldNameSet(response_header, response_header_location, field_location, name.c_str(), name.size());
    TSMimeHdrFieldAppend(response_header, response_header_location, field_location);
    TSMimeHdrFieldValueStringSet(response_header, response_header_location, field_location,
                                 -1, value.c_str(), value.size());
    // TODO(oschaaf): Do we need additional bookkeeping here?
  } else {
    TSError("field creation error for field [%s]", name.c_str());
    return;
  }
}

/**
  Add the value of host field to url. This is useful when 
  the filter tries to generate the original address entered in
  the browser bar for redirect purposes.

  @param hostname to be set in the filed, NULL default value
         means to use the hostname in request header.
*/
void 
TransactionMuncher::set_url_host(string* hostname)
{
  if (!request_header)
    if (TSHttpTxnClientReqGet(trans_txnp, &request_header, &header_location) != TS_SUCCESS) {
      TSError("couldn't retrieve client response header\n");
      throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
    }

  if (!hostname) {//use the hostname in the header
    retrieve_parts(TransactionMuncher::HOST); //make sure the host is retrieved
    hostname = &(cur_trans_parts[TransactionMuncher::HOST]);
  }
    
  TSMLoc url_loc;
  if (TSHttpHdrUrlGet(request_header, header_location, &url_loc) != TS_SUCCESS) {
    TSError("couldn't retrieve request url\n");
    TSHandleMLocRelease(request_header, header_location, url_loc);
    throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
  }

  if (TSUrlHostSet(request_header, url_loc, hostname->c_str(), hostname->length()) != TS_SUCCESS) {
    TSError("couldn't manipulate url field.\n");
    TSHandleMLocRelease(request_header, header_location, url_loc);
    throw TransactionMuncher::HEADER_RETRIEVAL_ERROR;
  }

  //update the url.
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

TransactionMuncher::TransactionMuncher(TSHttpTxn cur_txnp)
  :trans_txnp(cur_txnp), valid_parts(0), request_header(NULL), 
   response_header(NULL),header_location(NULL),response_header_location(NULL)
{
  update_validity_status();
}

/**
   destructor releasing the header buffer
   called manually to let TS to manage the release process
 */
TransactionMuncher::~TransactionMuncher()
{
  if (request_header)
    TSHandleMLocRelease(request_header, TS_NULL_MLOC, header_location);

  if (response_header)
    TSHandleMLocRelease(response_header, TS_NULL_MLOC, response_header_location);

}
