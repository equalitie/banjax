/*
 * Functions deal with the regex banning.
 *
 * Copyright (c) eQualit.ie 2015 under GNU AGPL v3.0 or later
 *
 *  Vmon: Dec 2015, Initial version
 */
#include <sys/time.h>

#include "denialator.h"
#include "ip_database.h"
#include "global_white_list.h"
#include "print.h"

using namespace std;

/*
 * checks if the ip has been reported to swabber and denial
 */
FilterResponse Denialator::on_http_request(const TransactionParts& transaction_parts)
{
  std::string cur_ip = transaction_parts.at(TransactionMuncher::IP);

  if (global_white_list && global_white_list->is_white_listed(cur_ip)) {
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
  }

  boost::optional<SwabberInterface::IpDb::IpState> ip_state = swabber_ip_db->get_ip_state(cur_ip);

  if (!ip_state) {
    print::debug("Denialator: doing nothing due to a failure to acquire ip db lock");
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
  }

  if (*ip_state != 0) {
    // If grace period has passed report to swabber
    timeval cur_time; gettimeofday(&cur_time, NULL);

    if ((cur_time.tv_sec - *ip_state) >= banning_grace_period) {
        print::debug("Denialator: Grace period passed, re-reporting to swabber");
        swabber_interface->ban(cur_ip, "flagged on " + to_string(*ip_state) + ", grace period passed. reported by denialator");
    }

    print::debug("Denialator: denying access to tagged ip: " ,cur_ip);

    // Record the first request for banning
    return FilterResponse([&](const TransactionParts& a, const FilterResponse& b) { return this->generate_response(a,b); });
  }

  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
}

std::string Denialator::generate_response(const TransactionParts&, const FilterResponse&)
{
  return "<html><header></header><body>504 Gateway Timeout</body></html>";
}
