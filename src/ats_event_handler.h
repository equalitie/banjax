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
class Banjax;

class ATSEventHandler
{
  friend class Banjax;
protected:
  static Banjax* banjax;

public:
  /**
     runs all filters to make decsion based on request header
     
     @param cd   Banjax continuation which include Banjax main object 
                 where all filters are accesible through it

  */
  static void handle_request(BanjaxContinuation* cd);

  /**
     runs filters who need to be executed on during generating the transaction's
     response
  */
  static void handle_response(BanjaxContinuation* cd);

  static int  banjax_global_eventhandler(TSCont contp, TSEvent event, void *edata);


  /**
     this is to reload banjax config when you get into the traffi_line -x
     situation 
   */
  static int  banjax_management_handler(TSCont contp, TSEvent event, void *edata);

  static void handle_http_close(Banjax::TaskQueue& current_queue, BanjaxContinuation* cd);

  /**
     Destroy the continuation and release the object related to it after
     transaction ends
   */
  static void destroy_continuation(TSCont contp);
};

//extern Banjax* ATSEventHandler::banjax;

#endif /* ats_event_handler.h */
