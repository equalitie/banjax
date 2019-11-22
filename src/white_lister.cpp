/*
 * Functions deal with the ip white listing.
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: Oct 2013, Initial version
 */
#include <string>
#include <list>

#include <ts/ts.h>

using namespace std;
#include "util.h"
#include "white_lister.h"
#include "print.h"

WhiteLister::WhiteLister(const YAML::Node& cfg,
                         GlobalWhiteList& white_list) :
  BanjaxFilter::BanjaxFilter(WHITE_LISTER_FILTER_ID, WHITE_LISTER_FILTER_NAME),
  white_list(white_list)
{
  queued_tasks[HTTP_REQUEST] = this;

  TSDebug(BANJAX_PLUGIN_NAME, "Loading white lister manager conf");

  try
  {
    YAML::Node white_listed_ips = cfg["white_listed_ips"];

    for (auto entry : white_listed_ips) {

      if (entry.IsMap()) {
        auto host  = entry["host"].as<std::string>();
        auto range = make_mask_for_range(entry["ip_range"].as<std::string>());

        white_list.insert(host, range);

        DEBUG("White listing ip range: ", range, " for ", host);
      }
      else {
        SubnetRange range = make_mask_for_range(entry.as<std::string>());

        white_list.insert(range);

        DEBUG("White listing ip range: ", range);
      }
    }
  }
  catch(std::exception& e)
  {
    TSDebug(BANJAX_PLUGIN_NAME, "Error loading white lister config: %s", e.what());
    throw;
  }

  TSDebug(BANJAX_PLUGIN_NAME, "Done loading white lister manager conf");
}

FilterResponse WhiteLister::on_http_request(const TransactionParts& transaction_parts)
{
  const auto& ip   = transaction_parts.at(TransactionMuncher::IP);
  const auto& host = transaction_parts.at(TransactionMuncher::HOST);

  if (auto hit_range = white_list.is_white_listed(host, ip)) {
    TSDebug(BANJAX_PLUGIN_NAME, "white listed ip: %s in range %X",
        ip.c_str(),
        hit_range->first);

    return FilterResponse(FilterResponse::SERVE_IMMIDIATELY_DONT_CACHE);
  }

  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
}


