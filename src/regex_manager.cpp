/*
 * Functions deal with the regex banning.
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: June 2013, Initial version
 */
#include <string>
#include <list>
#include <vector>
#include <sys/time.h>
#include <re2/re2.h>
#include <ts/ts.h>

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
    TSDebug(BANJAX_PLUGIN_NAME, "Setting regex re2 options");
    RE2::Options opt;
    opt.set_log_errors(false);
    opt.set_perl_classes(true);
    opt.set_posix_syntax(true);

    TSDebug(BANJAX_PLUGIN_NAME, "Loading regex manager conf");

    //now we compile all of them and store them for later use
    for(auto it = cfg.begin(); it != cfg.end(); ++it) {
      string cur_rule = (*it)["rule"].as<std::string>();

      TSDebug(BANJAX_PLUGIN_NAME, "initiating rule %s", cur_rule.c_str());

      unsigned int observation_interval = (*it)["interval"].as<unsigned int>();
      unsigned int threshold  = (*it)["hits_per_interval"].as<unsigned int>();

      rated_banning_regexes.emplace_back(
          new RatedRegex(rated_banning_regexes.size(),
                         cur_rule,
                         new RE2(((*it)["regex"].as<std::string>()), opt),
                         observation_interval * 1000,
                         threshold /(double)(observation_interval* 1000)));
    }

    total_no_of_rules = rated_banning_regexes.size();
  }
  catch(YAML::RepresentationException& e)
  {
    TSDebug(BANJAX_PLUGIN_NAME, "Error loading regex manager conf [%s].", e.what());
    throw;
  }

  TSDebug(BANJAX_PLUGIN_NAME, "Done loading regex manager conf");
}

/**
  applies all regex to an ATS record

  @param ats_record: the full request record including time url agent etc
  @return: 1 match 0 not match < 0 error.
*/
pair<RegexManager::RegexResult,RegexManager::RatedRegex*>
RegexManager::parse_request(string ip, string ats_record) const
{
  boost::optional<IpDb::IpState> ip_state;

  for(auto it=rated_banning_regexes.begin(); it != rated_banning_regexes.end(); it++) {
    if (RE2::FullMatch(ats_record, *((*it)->re2_regex))) {
        TSDebug(BANJAX_PLUGIN_NAME, "requests matched %s", ((*it)->re2_regex->pattern()).c_str());
        //if it is a simple regex i.e. with rate 0 we bans immidiately without
        //wasting time and mem
        if ((*it)->rate == 0) {
          TSDebug(BANJAX_PLUGIN_NAME, "simple regex, ban immediately");
          return make_pair(REGEX_MATCHED, (*it).get());
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
        RegexState cur_ip_and_regex_state;
        cur_ip_and_regex_state = (*ip_state)[(*it)->id];

        //if it is 0, then we don't have a record for
        //the current regex
        if (cur_ip_and_regex_state.begin_msec == 0) {
          cur_ip_and_regex_state.begin_msec = cur_time_msec;
        }

        //now we have a record, update the rate and ban if necessary.
        //we move the interval by the differences of the "begin_in_ms - cur_time_msec - interval*1000"
        //if it is less than zero we don't do anything just augument the rate
        long time_window_movement = cur_time_msec - cur_ip_and_regex_state.begin_msec - (*it)->interval;
        if (time_window_movement > 0) { //we need to move
           cur_ip_and_regex_state.begin_msec += time_window_movement;
           cur_ip_and_regex_state.rate = cur_ip_and_regex_state.rate - (cur_ip_and_regex_state.rate * time_window_movement - 1)/(double) (*it)->interval;
           cur_ip_and_regex_state.rate = cur_ip_and_regex_state.rate < 0 ? 1/(double) (*it)->interval : cur_ip_and_regex_state.rate; //if time_window_movement > time_window_movement(*it)->interval, then we just wipe history and start as a new interval
         }
         else {
           //we are still in the same interval so just increase the hit by 1
           cur_ip_and_regex_state.rate += 1/(double) (*it)->interval;
         }

        TSDebug(BANJAX_PLUGIN_NAME, "with rate %f /msec", cur_ip_and_regex_state.rate);

        (*ip_state)[(*it)->id] = cur_ip_and_regex_state;

        if (cur_ip_and_regex_state.rate >= (*it)->rate) {
          TSDebug(BANJAX_PLUGIN_NAME, "exceeding excessive rate %f /msec", (*it)->rate);
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
          return make_pair(REGEX_MATCHED, (*it).get());
        }
    }
  }

  //if we managed to get/make a valid state we are going to store it
  if (ip_state && ip_state->size() > 0)
    regex_manager_ip_db->set_ip_state(ip, *ip_state);

  //no match
  return make_pair(REGEX_MISSED, (RatedRegex*)NULL);
}

FilterResponse RegexManager::on_http_request(const TransactionParts& transaction_parts)
{
  const string sep(" ");
  TransactionParts ats_record_parts = (TransactionParts) transaction_parts;

  string ats_record =  ats_record_parts[TransactionMuncher::METHOD] + sep;
  ats_record+= ats_record_parts[TransactionMuncher::URL] + sep;
  ats_record+= ats_record_parts[TransactionMuncher::HOST] + sep;
  ats_record+= ats_record_parts[TransactionMuncher::UA];

  TSDebug(BANJAX_PLUGIN_NAME, "Examining '%s' for banned matches", ats_record.c_str());
  pair<RegexResult,RatedRegex*> result = parse_request(
						ats_record_parts[TransactionMuncher::IP],
						ats_record
						);

  if (result.first == REGEX_MATCHED) {
    TSDebug(BANJAX_PLUGIN_NAME, "asking swabber to ban client ip: %s", ats_record_parts[TransactionMuncher::IP].c_str());

    //here instead we are calling nosmos's banning client
    string ats_rec_comma_sep =
      ats_record_parts[TransactionMuncher::METHOD] + ", " +
      encapsulate_in_quotes(ats_record_parts[TransactionMuncher::URL]) + ", " +
      ats_record_parts[TransactionMuncher::HOST] + ", " +
      encapsulate_in_quotes(ats_record_parts[TransactionMuncher::UA]);

    string banning_reason = "matched regex rule " + result.second->rule_name + ", " + ats_rec_comma_sep;

    swabber_interface->ban(ats_record_parts[TransactionMuncher::IP], banning_reason);
    return FilterResponse([&](const TransactionParts& a, const FilterResponse& b) { return this->generate_response(a, b); });

  } else if (result.first != REGEX_MISSED) {
    TSError("Regex failed with error: %d\n", result.first);
  }

  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);

}

std::string RegexManager::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{
  (void)transaction_parts; (void)response_info;
  return forbidden_message;
}
