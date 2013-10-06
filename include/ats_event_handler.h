/*
*	part of regex_ban plugin: set of callbacks being called by ATS
*   as most of the functions are call back this class is more like
*   a collection of static functions
*
*       Vmon: May 2013: Initial version.
*/

#ifndef ATS_EVENT_HANDLER_H
#define ATS_EVENT_HANDLER_H

#define MAX_URL_LENGTH        4096;
#define MAX_UA_LENGTH         4096;
#define MAX_REQUEST_LENGTH    16384;
#define MAX_COOKIE_LENGTH     8192;

class BanjaxContinuation;
class ATSEventHandler
{
 public:
  static void handle_txn_start(TSCont contp, TSHttpTxn txnp);
  static void destroy_continuation(TSCont contp);

  /**
     runs all filters to make decsion based on request header
     
     @param cd   Banjax continuation which include Banjax main object 
                 where all filters are accesible through it

  */
  static void handle_request(BanjaxContinuation* cd);
  static void handle_response(BanjaxContinuation* cd);
  static int  banjax_global_eventhandler(TSCont contp, TSEvent event, void *edata);

};

#endif /* ats_event_handler.h */

