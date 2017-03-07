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

class BanjaxContinuation{
public:
  TSHttpTxn txnp;
  TSCont contp;

  TransactionMuncher transaction_muncher;
  FilterResponse response_info;

  // The filter that genenates the response.
  BanjaxFilter* responding_filter;

  ~BanjaxContinuation();

  /**
     Constructor to set the default values
   */
  BanjaxContinuation(TSHttpTxn cur_txn)
    : txnp(cur_txn), transaction_muncher(cur_txn)
  { }
};

#endif /*banjax_continuation.h*/
