/*
 * the continuation data assigned to each request along side with the functions 
 * to manipulate them
 *
 * Vmon: June 2013
 */
#ifndef BANJAX_CONTINUATION_H
#define BANJAX_CONTINUATION_H

#include "banjax_filter.h"
#include "transaction_muncher.h"

class Banjax;

class BanjaxContinuation{

 public:
  enum calling_func
  {
    HANDLE_RESPONSE,
    HANDLE_REQUEST,
    READ_REGEX_LIST
  } cf;

  TSHttpTxn txnp;
  TSCont contp;

  Banjax* cur_banjax_inst;
  TransactionMuncher transaction_muncher;
  FilterResponse* response_info;

  //the filter that genenates the response
  BanjaxFilter* responding_filter;
  //the function that should generate the response
  std::string (BanjaxFilter::*response_generator)(const TransactionParts& transaction_parts, FilterResponse* response_info);

  /* Destructor: Destroys the continuation after response
     has been served */
  ~BanjaxContinuation();

  /**
     Constructor to set the default values
   */
  BanjaxContinuation(TSHttpTxn cur_txn)
    : txnp(cur_txn), transaction_muncher(cur_txn), response_info(NULL), response_generator(NULL)
      
    {
    }
};

#endif /*banjax_continuation.h*/
