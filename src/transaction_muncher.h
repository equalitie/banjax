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

#ifndef TRANSACTION_MUNCHER_H
#define TRANSACTION_MUNCHER_H

#include <ts/ts.h>
#include <map>
#include <string>
#include <stdint.h>
//We are going to store parts in an std::string. If that appears to much
//waste of time, then we will store them as (pointer, length) pairs.
typedef std::map<uint64_t, std::string> TransactionParts;

class TransactionMuncher{
 protected:
  TransactionParts cur_trans_parts;
  TSHttpTxn trans_txnp;
  uint64_t valid_parts; //This flag collection indicate the parts we have
                        //already retrieved

  TSMBuffer request_header, response_header;
  TSMLoc header_location, response_header_location;

  /**
     if the response_header pointer is NULL we call this function
     internally to retrieve the response header. Throws exception
     if it faces error.
   */
  void retrieve_response_header();

 public:
    enum TransactionPart{
      VALIDITY_STAT  = 0x000,  //This flag collection indicate the parts we have
                               //already retrieved
      IP             = 0x001,
      URL            = 0x002,
      HOST           = 0x004,
      UA             = 0x008,
      COOKIE         = 0x010,
      URL_WITH_HOST  = 0x020,
      METHOD         = 0x040,
      MISS           = 0x080,
      STATUS         = 0x100,
      PROTOCOL       = 0x200,
      CONTENT_LENGTH = 0x400
    };

    enum PARTS_ERROR {
      HEADER_RETRIEVAL_ERROR,
      PART_RETRIEVAL_ERROR
    };

    /**
       provide the vessel to tell the fliters that  telling which parts has been 
       retrieved successfully
       it updates the VALIDITY_STAT in transaction_parts map
     */
    inline void update_validity_status()
    {
      std::string valbuf((char*)&valid_parts, sizeof(uint64_t));
      cur_trans_parts[VALIDITY_STAT] = valbuf;
    }

    /**
       checks if the parts are already retrieved and if not ask ATS 
       to fullfill the part. Through exception if it fails

       @param a set of flags of TransactionPart type indicate what part of 
              GET transaction is needed
              
       @return A map of transaction parts
     */
    const TransactionParts& retrieve_parts(uint64_t requested_log_parts);

    /**
       same as retrieve_parts but needs to be called in reponse handle 
       cause it uses response_header
     */
    const TransactionParts&  retrieve_response_parts(uint64_t requested_log_parts);

    /**
       Should be called during response to set the header status to
       something meaningful
       
       @param status the HTTP status
    */
    void set_status(TSHttpStatus status);

    /**
       Appends a name/value to the response headers
       
       @param name The name of the response header
       @param value The value of the response header       
    */
    void append_header(const std::string& name, const std::string& value);
    
    /**
       Add the value of host field to url. This is useful when 
       the filter tries to generate the original address entered in
       the browser bar for redirect purposes.

       @param hostname to be set in the filed, NULL default value
              means to use the hostname in request header.
    */
    void set_url_host(std::string* hostname = NULL);

    /**
       Indicate that the transction missed the cache. Should be 
       called by ATSEventHandler when the hook for missing the 
       cache is called.
     */
    void miss()
    {
      cur_trans_parts[MISS] = std::string();
    }
    /**
       Constructor
       
       @cur_txnp the transaction pointer whose parts we want to retrieve
     */
    TransactionMuncher(TSHttpTxn cur_txnp);

    /**
       destructor releasing the header buffer
    */
    ~TransactionMuncher();
};

#endif // transaction_muncher.h 
