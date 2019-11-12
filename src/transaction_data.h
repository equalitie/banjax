/*
 * the continuation data assigned to each request along side with the functions 
 * to manipulate them
 *
 * Vmon: June 2013
 */
#ifndef BANJAX_CONTINUATION_H
#define BANJAX_CONTINUATION_H

#include "banjax.h"
#include "banjax_filter.h"
#include "transaction_muncher.h"

class Banjax;

class TransactionData{
public:
  /**
     Constructor to set the default values
   */
  TransactionData(std::shared_ptr<Banjax> banjax, TSHttpTxn cur_txn)
    : banjax(std::move(banjax))
    , txnp(cur_txn)
    , transaction_muncher(cur_txn)
  { }

  static
  int handle_transaction_change(TSCont contp, TSEvent event, void *edata);

private:
  std::shared_ptr<Banjax> banjax;
  TSHttpTxn txnp;

  TransactionMuncher transaction_muncher;
  FilterResponse response_info;

private:
  void handle_request();
  void handle_response();
  void handle_http_close(Banjax::TaskQueue& current_queue);
};

#endif /*banjax_continuation.h*/
