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
  uint64_t valid_parts; //This flag collection indicate the parts we have
                        //already retrieved

  TSHttpTxn trans_txnp;
  TSMBuffer request_header;
  TSMLoc header_location;

 public:
    enum TransactionPart{
        IP        = 0x01,
        URL       = 0x02,
        HOST      = 0x04,
        UA        = 0x08,
        COOKIE    = 0x10
    };

    enum PARTS_ERROR {
      HEADER_RETRIEVAL_ERROR,
      PART_RETRIEVAL_ERROR
    };


    /**
       checks if the parts are already retrieved and if not ask ATS 
       to fullfill the part. Through exception if it fails

       @param a set of flags of TransactionPart type indicate what part of 
              GET transaction is needed
              
       @return A map of transaction parts
     */
    const TransactionParts& retrieve_parts(uint64_t requested_log_parts);

    /**
       Should be called during response to set the header status to
       something meaningful
       
       @param status the HTTP status
    */
    void set_status(TSHttpStatus status);

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
