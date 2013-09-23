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

BanjaxContinuation::~BanjaxContinuation()
{
  // TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);

  // TSCont temp_contp = contp;
  TSDebug("banjax", "somebody is destorynig the continuation");
  
  // TSfree(this);
  // TSContDestroy(temp_contp);

}
