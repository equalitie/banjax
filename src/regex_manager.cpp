/*
 * Functions deal with the regex banning.
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: June 2013, Initial version
 */
#include <vector>
#include <sys/time.h>

#include "regex_manager.h"

using namespace std;

/**
  reads all the regular expressions from the database.
  and compile them
 */
void RegexManager::load_config()
{
  try
  {
    print::debug("RegexManager::load_config");

    RE2::Options opt;
    opt.set_log_errors(false);
    opt.set_perl_classes(true);
    opt.set_posix_syntax(true);

    for (const auto& node : cfg) {
      string cur_rule = node["rule"].as<std::string>();

      print::debug("RegexManager: Initiating rule ", cur_rule);

      unsigned int observation_interval = node["interval"].as<unsigned int>();
      unsigned int threshold = node["hits_per_interval"].as<unsigned int>();

      rated_banning_regexes.emplace_back(
          new RatedRegex(rated_banning_regexes.size(),
                         cur_rule,
                         new RE2(node["regex"].as<std::string>(), opt),
                         observation_interval * 1000,
                         threshold /(double)(observation_interval* 1000)));
    }

    total_no_of_rules = rated_banning_regexes.size();
  }
  catch(YAML::RepresentationException& e)
  {
    print::debug("RegexManager: Error loading regex manager conf [",e.what(),"].");
    throw;
  }
}

RegexManager::RatedRegex*
RegexManager::try_match(string ip, string ats_record) const
{
  boost::optional<IpDb::IpState> ip_state;

  for(const auto& rule : rated_banning_regexes) {
    if (RE2::FullMatch(ats_record, *(rule->re2_regex))) {
      print::debug("RegexManager: Request matched ", rule->re2_regex->pattern());
      //if it is a simple regex i.e. with rate 0 we bans immidiately without
      //wasting time and mem
      if (rule->rate == 0) {
        print::debug("RegexManager: Simple regex, ban immediately");
        return rule.get();
      }
      //select appropriate rate, dependent on whether GET or POST request

      /* we need to check the rate condition here */
      //getting current time in msec
      timeval cur_time; gettimeofday(&cur_time, NULL);
      long cur_time_msec = cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000.0;

      /* first we check if we already have retreived the state for this ip */
      if (!ip_state || ip_state->size() == 0) {
        ip_state =  regex_manager_ip_db->get_ip_state(ip);
        if (!ip_state) //we failed to read the database we make no judgement
          continue;
      }

      //if we succeeded in retreiving but the size is zero then it meaens we don't have a record of this ip at all
      if (ip_state->size() == 0) {
        ip_state->resize(total_no_of_rules);
      }

      //now we check the begining of the hit for this regex (if the vector is just created everything is 0, in case
      //this is the first regex this ip hits)
      RegexState state = (*ip_state)[rule->id];

      //if it is 0, then we don't have a record for
      //the current regex
      if (state.begin_msec == 0) {
        state.begin_msec = cur_time_msec;
      }

      //now we have a record, update the rate and ban if necessary.
      //we move the interval by the differences of the "begin_in_ms - cur_time_msec - interval*1000"
      //if it is less than zero we don't do anything just augument the rate
      long time_window_movement = cur_time_msec - state.begin_msec - rule->interval;
      if (time_window_movement > 0) { //we need to move
        state.begin_msec += time_window_movement;
        state.rate = state.rate - (state.rate * time_window_movement - 1)/(double) rule->interval;
        state.rate = state.rate < 0 ? 1/(double) rule->interval : state.rate; //if time_window_movement > time_window_movementrule->interval, then we just wipe history and start as a new interval
      }
      else {
        //we are still in the same interval so just increase the hit by 1
        state.rate += 1/(double) rule->interval;
      }

      print::debug("RegexManager: with rate ",state.rate," /msec");

      (*ip_state)[rule->id] = state;

      if (state.rate >= rule->rate) {
        print::debug("Exceeding excessive rate ",rule->rate," /msec");
        //clear the record to avoid multiple reporting to swabber
        //we are not clearing the state cause it is not for sure that
        //swabber ban the ip due to possible failure of acquiring lock
        // ip_state.detail.begin_msec = 0;
        // ip_state.detail.rate = 0;
        // regex_manager_ip_db->set_ip_state(ip, ip_state.state_allocator);

        //before calling swabber we need to delete the memory as swabber will
        //delete the ip database entery and the memory will be lost.
        //However, if swabber fails to acquire a lock this means
        //the ip can escape the banning, this however is a miner
        //concern compared to the fact that we might ran out of
        //memory.

        regex_manager_ip_db->set_ip_state(ip, *ip_state);
        return rule.get();
      }
    }
  }

  //if we managed to get/make a valid state we are going to store it
  if (ip_state && ip_state->size() > 0)
    regex_manager_ip_db->set_ip_state(ip, *ip_state);

  //no match
  return nullptr;
}

FilterResponse RegexManager::on_http_request(const TransactionParts& transaction_parts)
{
  TransactionParts ats_record_parts = (TransactionParts) transaction_parts;

  string ats_record = ats_record_parts[TransactionMuncher::METHOD] + " "
                    + ats_record_parts[TransactionMuncher::URL]    + " "
                    + ats_record_parts[TransactionMuncher::HOST]   + " "
                    + ats_record_parts[TransactionMuncher::UA];

  print::debug("RegexManager: Examining '",ats_record,"' for banned matches");

  RatedRegex* result = try_match(
						ats_record_parts[TransactionMuncher::IP],
						ats_record);

  if (result) {
    print::debug("Asking swabber to ban client ip: ", ats_record_parts[TransactionMuncher::IP]);

    //here instead we are calling nosmos's banning client
    string banning_reason
      = "matched regex rule " + result->rule_name + ", "
      + ats_record_parts[TransactionMuncher::METHOD] + ", "
      + encapsulate_in_quotes(ats_record_parts[TransactionMuncher::URL]) + ", "
      + ats_record_parts[TransactionMuncher::HOST] + ", "
      + encapsulate_in_quotes(ats_record_parts[TransactionMuncher::UA]);

    swabber->ban(ats_record_parts[TransactionMuncher::IP], banning_reason);

    return FilterResponse([&](const TransactionParts& a, const FilterResponse& b) { return this->generate_response(a, b); });
  }

  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
}

std::string RegexManager::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{
  (void)transaction_parts; (void)response_info;
  return forbidden_message;
}
