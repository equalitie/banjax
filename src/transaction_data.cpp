/*
 * Continuation class methods
 *
 * Copyright (C) eQualit.ie under GNU AGPL v3.0 or later
 *
 * Vmon June 2013, Initial verison
 */
#include <ts/ts.h>

#include "transaction_data.h"

/**
   We are calling the destructor manually so we can ask 
   TS to release the memory according to their management
 */
TransactionData::~TransactionData()
{
  if (response_info.response_data != NULL && response_info.response_type == FilterResponse::I_RESPOND) {
    delete response_info.response_data;
    response_info.response_data = NULL;
  }
}
