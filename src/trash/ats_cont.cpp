/*
 * Continuation class methods
 *
 * Copyright (C) eQualit.ie under GNU AGPL v3.0 or later
 *
 * Vmon June 2013, Initial verison
 */
#include "ats_cont.h"
BanjaxContinuation::~BanjaxContinuation(TSHttpTxn txnp, TSCont contp)
{
  cdata *cd = NULL;

  cd = (cdata *) TSContDataGet(contp);
  if (cd != NULL) {
    TSfree(cd);
  }
  TSContDestroy(contp);
  TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
  return;
}
