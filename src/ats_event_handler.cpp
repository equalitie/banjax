/*
 * This class implmentats callbacks function that are called by ATS
 * AUTHORS:
 *   Vmon: May 2013, moving Bill's code to C++
 */

#include <stdio.h>
#include <ts/ts.h>
#include <regex.h>
#include <string.h>
#include <iostream>

#include <string>
#include <vector>
#include <list>

#include <zmq.hpp>
using namespace std;

#include <re2/re2.h>
//to retrieve the client ip
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//to run fail2ban-client
#include <stdlib.h>

#include "banjax.h"
#include "transaction_data.h"
#include "transaction_muncher.h"
#include "regex_manager.h"
#include "challenge_manager.h"
#include "swabber_interface.h"
#include "ats_event_handler.h"
#include "util.h"
#include "print.h"

int
ATSEventHandler::handle_transaction_change(TSCont contp, TSEvent event, void *edata)
{
  TSHttpTxn txnp = (TSHttpTxn) edata;
  TransactionData* cd = (TransactionData*) TSContDataGet(contp);

  switch (event) {
  case TS_EVENT_HTTP_READ_REQUEST_HDR:
    handle_request((TransactionData *) TSContDataGet(contp));
    return TS_EVENT_NONE;

  case TS_EVENT_HTTP_READ_CACHE_HDR:
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    return TS_EVENT_NONE;

  case TS_EVENT_HTTP_SEND_REQUEST_HDR:
    TSDebug(BANJAX_PLUGIN_NAME, "miss");
    cd->transaction_muncher.miss();
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    return TS_EVENT_NONE;

  case TS_EVENT_HTTP_SEND_RESPONSE_HDR:
    handle_response(cd);
    return TS_EVENT_NONE;

  case TS_EVENT_HTTP_TXN_CLOSE:
    TSDebug(BANJAX_PLUGIN_NAME, "txn close");
    handle_http_close(banjax->task_queues[BanjaxFilter::HTTP_CLOSE], cd);
    cd->~TransactionData();
    TSfree(cd);
    TSContDestroy(contp);
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    break;

  case TS_EVENT_TIMEOUT:
    TSDebug("banjaxtimeout", "timeout" );

  default:
    TSDebug(BANJAX_PLUGIN_NAME, "Unsolicitated event call?" );
    break;
  }

  return TS_EVENT_NONE;
}

void
ATSEventHandler::handle_request(TransactionData* cd)
{
  // Retreiving part of header requested by the filters
  const TransactionParts& cur_trans_parts = cd->transaction_muncher.retrieve_parts(banjax->which_parts_are_requested());

  bool continue_filtering = true;

  auto& http_request_filters = banjax->task_queues[BanjaxFilter::HTTP_REQUEST];

  for (auto filter : http_request_filters) {
    FilterResponse cur_filter_result = filter->on_http_request(cur_trans_parts);

    switch (cur_filter_result.response_type)
    {
      case FilterResponse::GO_AHEAD_NO_COMMENT:
        continue;

      case FilterResponse::SERVE_IMMIDIATELY_DONT_CACHE:
        //This is when the requester is white listed
        if (TSHttpTxnServerRespNoStoreSet(cd->txnp, true) != TS_SUCCESS) {
          TSDebug(BANJAX_PLUGIN_NAME, "Unable to make the response uncachable" );
        }

        continue_filtering = false;
        break;

      case FilterResponse::SERVE_FRESH:
        //Tell ATS not to cache this request, hopefully it means that
        //It shouldn't be served from the cache either.
        //TODO: One might need to investigate TSHttpTxnRespCacheableSet()
        if (TSHttpTxnServerRespNoStoreSet(cd->txnp, true) != TS_SUCCESS
            || TSHttpTxnConfigIntSet(cd->txnp, TS_CONFIG_HTTP_CACHE_HTTP, 0) != TS_SUCCESS)
          TSDebug(BANJAX_PLUGIN_NAME, "Unable to make the response uncachable" );
        break;

      case FilterResponse::I_RESPOND:
        // from here on, cur_filter_result is owned by the continuation data.
        cd->response_info = cur_filter_result;
        // TODO(oschaaf): commented this. @vmon: we already hook this globally,
        // is there a reason we need to hook it again here?
        //TSHttpTxnHookAdd(cd->txnp, TS_HTTP_SEND_RESPONSE_HDR_HOOK, cd->contp);
        TSHttpTxnReenable(cd->txnp, TS_EVENT_HTTP_ERROR);
        return;

      default:
        //Not implemeneted, hence ignore
        break;
    }

    if (!continue_filtering) break;
  }

  TSHttpTxnReenable(cd->txnp, TS_EVENT_HTTP_CONTINUE);
}

void
ATSEventHandler::handle_response(TransactionData* cd)
{
  auto status = cd->transaction_muncher.get_response_status();

  //we need to retrieve response parts for any filter who requested it.
  cd->transaction_muncher.retrieve_response_parts(banjax->which_response_parts_are_requested());

  auto set_error_body = [&](const string&  buf) {
    char* b = (char*) TSmalloc(buf.size());
    memcpy(b, buf.data(), buf.size());

    auto* rd = cd->response_info.response_data;

    char* content_type = rd ? rd->get_and_release_content_type()
                            : nullptr;

    TSHttpTxnErrorBodySet(cd->txnp, b, buf.size(), content_type);
  };

  if (cd->response_info.response_type == FilterResponse::I_RESPOND) {
    cd->transaction_muncher.set_status(TS_HTTP_STATUS_GATEWAY_TIMEOUT);
    std::string buf = cd->response_info.response_data->response_generator
                      (cd->transaction_muncher.retrieve_parts(banjax->all_filters_requested_part), cd->response_info);

    cd->transaction_muncher.set_status(
        (TSHttpStatus) cd->response_info.response_data->response_code);

    if (cd->response_info.response_data->set_cookie_header.size()) {
      cd->transaction_muncher.append_header(
          "Set-Cookie", cd->response_info.response_data->set_cookie_header.c_str());
    }

    // Forcefully disable cacheing
    cd->transaction_muncher.append_header("Expires", "Mon, 23 Aug 1982 12:00:00 GMT");
    cd->transaction_muncher.append_header("Cache-Control", "no-store, no-cache, must-revalidate");
    cd->transaction_muncher.append_header("Pragma", "no-cache");

    if (buf.size() == 0) {
      // When we get here, no valid response body was generated somehow.
      // Insert one, to prevent triggering an assert in TSHttpTxnErrorBodySet
      buf.append("Not authorized");
    }

    set_error_body(buf);
    TSHttpTxnReenable(cd->txnp, TS_EVENT_HTTP_CONTINUE);
  }
  else if (status == 110 /* = ERR_CONNECT_FAIL */) {
    // There was a ticket that some origins responded with this error.
    // Standard errors are handled nicely by TS but this one caused
    // TS to halt for approx 1 minute causing confusion about whether
    // the error was at TS side or at origin.
    string error_body = print::str(
      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"
      "        \"http://www.w3.org/TR/html4/strict.dtd\">\n"
      "<html>\n"
      "    <head>\n"
      "        <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\n"
      "        <title>Error response</title>\n"
      "    </head>\n"
      "    <body>\n"
      "        <h1>Error response</h1>\n"
      "        <p>Error code: ", status,"</p>\n"
      "        <p>Error code explanation: ERR_CONNECT_FAIL</p>\n"
      "    </body>\n"
      "</html>\n");

    set_error_body(error_body);
    TSHttpTxnReenable(cd->txnp, TS_EVENT_HTTP_ERROR);
  }
  else {
    TSHttpTxnReenable(cd->txnp, TS_EVENT_HTTP_CONTINUE);
  }
}

void
ATSEventHandler::handle_http_close(Banjax::TaskQueue& current_queue, TransactionData* cd)
{
  const TransactionParts& cur_trans_parts = cd->transaction_muncher.retrieve_parts(banjax->which_parts_are_requested());

  for(auto cur_task : current_queue) {
    cur_task->on_http_close(cur_trans_parts);
  }
}
