/*
 * Continuation class methods
 *
 * Copyright (C) eQualit.ie under GNU AGPL v3.0 or later
 *
 * Vmon June 2013, Initial verison
 */
#include <ts/ts.h>

//#include "banjax.h"
#include "banjax_continuation.h"

/**
   We are calling the destructor manually so we can ask 
   TS to release the memory according to their management
 */
BanjaxContinuation::~BanjaxContinuation()
{
  if (response_info.response_data != NULL && response_info.response_type == FilterResponse::I_RESPOND) {
    delete response_info.response_data;
    response_info.response_data = NULL;
  }
  // TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);

  // TSCont temp_contp = contp;
  // TSfree(this);
  // TSContDestroy(temp_contp);

}
