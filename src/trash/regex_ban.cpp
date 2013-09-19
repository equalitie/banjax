/*
*	regex_ban is a plugin designed to filter requests to block malicious and DDOS requests	
*
*	Initial version: Bill Doran 2013/04/14
*       Vmon: May 2013: Add writing into fail2ban socket. Moving to C++.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ts/ts.h>
#include <regex.h>

//to retrieve the client ip
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


//gdb 
#include <signal.h>

#include <banjax.h>

#define MAX_NREGEXES 10
#define RETRY_TIME 10

static void
destroy_continuation(TSHttpTxn txnp, TSCont contp)
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

int
check_ts_version()
{

  const char *ts_version = TSTrafficServerVersionGet();
  int result = 0;

  if (ts_version) {
    int major_ts_version = 0;
    int minor_ts_version = 0;
    int patch_ts_version = 0;

    if (sscanf(ts_version, "%d.%d.%d", &major_ts_version, &minor_ts_version, &patch_ts_version) != 3) {
      return 0;
    }

    /* Need at least TS 2.0 */
    if (major_ts_version >= 2) {
      result = 1;
    }

  }

  return result;
}


void
TSPluginInit(int argc, const char *argv[])
{
  int i;
  TSPluginRegistrationInfo info;
  TSReturnCode error;

  info.plugin_name = "banjax";
  info.vendor_name = "eQualit.ie";
  info.support_email = "info@deflect.ca";

  if (TSPluginRegister(TS_SDK_VERSION_3_0, &info) != TS_SUCCESS) {
    TSError("Plugin registration failed. \n");
  }

  if (!check_ts_version()) {
    TSError("Plugin requires Traffic Server 3.0 or later\n");
    return;
  }
/* create an TSTextLogObject to log blacklisted requests to */
  error = TSTextLogObjectCreate("regex_ban_plugin", TS_LOG_MODE_ADD_TIMESTAMP, &log);
  if (!log || error == TS_ERROR) {
    TSDebug("regex_ban_plugin", "error while creating log");
  }
  
  TSDebug("regex_ban_plugin", "in the beginning");
  regex_mutex = TSMutexCreate();

  nregexes = 0;
  for(i=0; i < MAX_NREGEXES; i++){
    regexes[i] = NULL;	
  }
  global_contp = TSContCreate(regex_ban_plugin, NULL/*regex_mutex*/);
  read_regex_list(global_contp);
  

  TSHttpHookAdd(/*TS_HTTP_READ_REQUEST_HDR_HOOK*/ TS_HTTP_TXN_START_HOOK, global_contp);
  //TS_EVENT_HTTP_READ_REQUEST_HDR TS_HTTP_READ_REQUEST_HDR_HOOK TSHttpTxn
}


