/*
 * Functions deal with the regex banning.
 *
 * Copyright (c) eQualit.ie 2015 under GNU AGPL v3.0 or later
 *
 *  Vmon: Dec 2015, Initial version
 */
#include <string>
#include <cstring> //memcpy
#include <sys/time.h>
#include <list>
#include <vector>
#include <utility>

#include <typeinfo>

#include <ts/ts.h>

using namespace std;

#include "denialator.h"
#include "ip_database.h"

/**
   do nothing
 */
void
Denialator::load_config()
{

}

/**
  checks if the ip has been reported to swabber and denial


*/
FilterResponse Denialator::execute(const TransactionParts& transaction_parts)
{

  std::string cur_ip = transaction_parts.at(TransactionMuncher::IP);
  std::pair<bool,FilterState> cur_ip_state(ip_database->get_ip_state(cur_ip, SWABBER_INTERFACE_ID));

  /* If we failed to query the database then just don't report to swabber */
  if (cur_ip_state.first == false) {
  /* If it is zero size we set it to the current time */
    TSDebug(BANJAX_PLUGIN_NAME, "denialotr not doing anything to failure of aquiring ip db lock ");
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
  }

  if (cur_ip_state.second.size() != 0) { //oh oh you have been reported
    //if grace period is passed report to swabber
    //check if we need to report to swabber
    timeval cur_time; gettimeofday(&cur_time, NULL);
    if ((cur_time.tv_sec - cur_ip_state.second[0]) >= banning_grace_period) {
        TSDebug(BANJAX_PLUGIN_NAME, "grace period passed, re-reporting to swabber");
        swabber_interface->ban(cur_ip, "flagged on " + to_string(cur_ip_state.second[0]) + ", grace period passed. reported by denialator");
    }

    TSDebug(BANJAX_PLUGIN_NAME, "denialotr denying access to tagged ip: %s ",cur_ip.c_str());
    //recording the first request for banning
    return FilterResponse(static_cast<ResponseGenerator>(&Denialator::generate_response));

  }


  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);

}

std::string Denialator::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{
  (void)transaction_parts; (void)response_info;
  char* forbidden_response = new char[forbidden_message_length+1];
  memcpy((void*)forbidden_response, (const void*)forbidden_message.c_str(), (forbidden_message_length+1)*sizeof(char));
  return forbidden_response;
  
}
