/*
 * Copyright (C) eQualit.ie under GNU AGPL v3.0 or later
 */

#include <string>
#include "transaction_data.h"
#include "print.h"

using namespace std;

int
TransactionData::handle_transaction_change(TSCont contp, TSEvent event, void *edata)
{
  TSHttpTxn txnp = (TSHttpTxn) edata;
  TransactionData* self = (TransactionData*) TSContDataGet(contp);

  switch (event) {
  case TS_EVENT_HTTP_READ_REQUEST_HDR:
    self->handle_request();
    return TS_EVENT_NONE;

  case TS_EVENT_HTTP_READ_CACHE_HDR:
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    return TS_EVENT_NONE;

  case TS_EVENT_HTTP_SEND_REQUEST_HDR:
    TSDebug(BANJAX_PLUGIN_NAME, "miss");
    self->transaction_muncher.miss();
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    return TS_EVENT_NONE;

  case TS_EVENT_HTTP_SEND_RESPONSE_HDR:
    self->handle_response();
    return TS_EVENT_NONE;

  case TS_EVENT_HTTP_TXN_CLOSE:
    TSDebug(BANJAX_PLUGIN_NAME, "txn close");
    self->handle_http_close(self->banjax->task_queues[BanjaxFilter::HTTP_CLOSE]);
    self->~TransactionData();
    TSfree(self);
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
TransactionData::handle_request()
{
  // Retreiving part of header requested by the filters
  const TransactionParts& cur_trans_parts = transaction_muncher.retrieve_parts(banjax->which_parts_are_requested());

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
        if (TSHttpTxnServerRespNoStoreSet(txnp, true) != TS_SUCCESS) {
          TSDebug(BANJAX_PLUGIN_NAME, "Unable to make the response uncachable" );
        }

        continue_filtering = false;
        break;

      case FilterResponse::SERVE_FRESH:
        //Tell ATS not to cache this request, hopefully it means that
        //It shouldn't be served from the cache either.
        //TODO: One might need to investigate TSHttpTxnRespCacheableSet()
        if (TSHttpTxnServerRespNoStoreSet(txnp, true) != TS_SUCCESS
            || TSHttpTxnConfigIntSet(txnp, TS_CONFIG_HTTP_CACHE_HTTP, 0) != TS_SUCCESS)
          TSDebug(BANJAX_PLUGIN_NAME, "Unable to make the response uncachable" );
        break;

      case FilterResponse::I_RESPOND:
        // from here on, cur_filter_result is owned by the continuation data.
        response_info = cur_filter_result;
        // TODO(oschaaf): commented this. @vmon: we already hook this globally,
        // is there a reason we need to hook it again here?
        //TSHttpTxnHookAdd(txnp, TS_HTTP_SEND_RESPONSE_HDR_HOOK, contp);
        TSHttpTxnReenable(txnp, TS_EVENT_HTTP_ERROR);
        return;

      default:
        //Not implemeneted, hence ignore
        break;
    }

    if (!continue_filtering) break;
  }

  TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
}

void
TransactionData::handle_response()
{
  auto status = transaction_muncher.get_response_status();

  //we need to retrieve response parts for any filter who requested it.
  transaction_muncher.retrieve_response_parts(banjax->which_response_parts_are_requested());

  auto set_error_body = [&](const string&  buf) {
    char* b = (char*) TSmalloc(buf.size());
    memcpy(b, buf.data(), buf.size());

    auto* rd = response_info.response_data.get();

    char* content_type = rd ? rd->get_and_release_content_type()
                            : nullptr;

    TSHttpTxnErrorBodySet(txnp, b, buf.size(), content_type);
  };

  if (response_info.response_type == FilterResponse::I_RESPOND) {
    transaction_muncher.set_status(TS_HTTP_STATUS_GATEWAY_TIMEOUT);
    std::string buf = response_info.response_data->response_generator
                      (transaction_muncher.retrieve_parts(banjax->all_filters_requested_part), response_info);

    transaction_muncher.set_status(
        (TSHttpStatus) response_info.response_data->response_code);

    if (response_info.response_data->set_cookie_header.size()) {
      transaction_muncher.append_header(
          "Set-Cookie", response_info.response_data->set_cookie_header.c_str());
    }

    // Forcefully disable cacheing
    transaction_muncher.append_header("Expires", "Mon, 23 Aug 1982 12:00:00 GMT");
    transaction_muncher.append_header("Cache-Control", "no-store, no-cache, must-revalidate");
    transaction_muncher.append_header("Pragma", "no-cache");

    if (buf.size() == 0) {
      // When we get here, no valid response body was generated somehow.
      // Insert one, to prevent triggering an assert in TSHttpTxnErrorBodySet
      buf.append("Not authorized");
    }

    set_error_body(buf);
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
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
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_ERROR);
  }
  else {
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
  }
}

void
TransactionData::handle_http_close(Banjax::TaskQueue& current_queue)
{
  const TransactionParts& cur_trans_parts = transaction_muncher.retrieve_parts(banjax->which_parts_are_requested());

  for(auto cur_task : current_queue) {
    cur_task->on_http_close(cur_trans_parts);
  }
}
