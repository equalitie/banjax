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
#include <stdio.h>
#include <sys/time.h>
#include <zmq.hpp>

#include <re2/re2.h> //google re2

#include <ts/ts.h>

using namespace std;

#include "regex_manager.h"
#include "ip_database.h"

/**
  reads all the regular expressions from the database.
  and compile them
 */
void
RegexManager::load_config(libconfig::Setting& cfg)
{
   try
   {
     const libconfig::Setting &banned_regexes_list = cfg["banned_regexes"];
 
     unsigned int count = banned_regexes_list.getLength();

     //now we compile all of them and store them for later use
     for(unsigned int i = 0; i < count; i++) {
       string cur_rule = (const char*) banned_regexes_list[i]["rule"];
       TSDebug(BANJAX_PLUGIN_NAME, "initiating rule %s", cur_rule.c_str());

       unsigned int observation_interval = banned_regexes_list[i]["interval"];
       unsigned int threshold  = banned_regexes_list[i]["hits_per_interval"];
       
       rated_banning_regexes.push_back(new RatedRegex(cur_rule, new RE2((const char*)(banned_regexes_list[i]["regex"])), observation_interval * 1000, threshold /(double)(observation_interval* 1000)));
       
     }

    }
   catch(const libconfig::SettingNotFoundException &nfex)
     {
       // Ignore.
     }
 
}

/**
  applies all regex to an ATS record

  @param ats_record: the full request record including time url agent etc
  @return: 1 match 0 not match < 0 error.
*/
pair<RegexManager::RegexResult,RatedRegex*>
RegexManager::parse_request(string ip, string ats_record, string method)
{
  for(list<RatedRegex*>::iterator it=rated_banning_regexes.begin(); it != rated_banning_regexes.end(); it++) {
      if (RE2::FullMatch(ats_record, *((*it)->re2_regex))) {
        TSDebug(BANJAX_PLUGIN_NAME, "requests matched %s", (char*)((*it)->re2_regex->pattern()).c_str());  
        //if it is a simple regex i.e. with rate 0 we bans immidiately without
        //wasting time and mem
        if ((*it)->rate == 0) {
            TSDebug(BANJAX_PLUGIN_NAME, "simple regex, ban immidiately");
            return make_pair(REGEX_MATCHED, (*it));
        }
	//select appropriate rate, dependent on whether GET or POST request
	int METHOD_TYPE;
	if (method.find("GET") != std::string::npos) {
	  METHOD_TYPE = 0;	
	} else {
	  METHOD_TYPE = 1;
        }
        /* we need to check the rate condition here */
        //getting current time in msec
        timeval cur_time; gettimeofday(&cur_time, NULL);
        long cur_time_msec = cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000.0;
          
        /* first we check if we already have a state for this ip */
        RegexBannerStateUnion cur_ip_state;
        cur_ip_state.state_allocator =  ip_database->get_ip_state(ip, REGEX_BANNER_FILTER_ID);
        if (cur_ip_state.detail[METHOD_TYPE].begin_msec == 0) {//We don't have a record 
          cur_ip_state.detail[METHOD_TYPE].begin_msec = cur_time_msec;
          cur_ip_state.detail[METHOD_TYPE].rate = 0;
          ip_database->set_ip_state(ip, REGEX_BANNER_FILTER_ID, cur_ip_state.state_allocator);

        } else { //we have a record, update the rate and ban if necessary.
          //we move the interval by the differences of the "begin_in_ms - cur_time_msec - interval*1000"
          //if it is less than zero we don't do anything
 
          long time_window_movement = cur_time_msec - cur_ip_state.detail[METHOD_TYPE].begin_msec - (*it)->interval;
          if (time_window_movement > 0) { //we need to move
            cur_ip_state.detail[METHOD_TYPE].begin_msec += time_window_movement;
            cur_ip_state.detail[METHOD_TYPE].rate= cur_ip_state.detail[METHOD_TYPE].rate - (cur_ip_state.detail[METHOD_TYPE].rate * time_window_movement - 1)/(double) (*it)->interval;
            cur_ip_state.detail[METHOD_TYPE].rate =  cur_ip_state.detail[METHOD_TYPE].rate < 0 ? 0 : cur_ip_state.detail[METHOD_TYPE].rate; //just to make sure
          }
          else {
            //we are still in the same interval so just increase the hit by 1
            cur_ip_state.detail[METHOD_TYPE].rate += 1/(double) (*it)->interval;
          }
          TSDebug(BANJAX_PLUGIN_NAME, "with rate %f /msec", cur_ip_state.detail[METHOD_TYPE].rate);
          ip_database->set_ip_state(ip, REGEX_BANNER_FILTER_ID, cur_ip_state.state_allocator);
        }
        if (cur_ip_state.detail[METHOD_TYPE].rate >= (*it)->rate) {
          TSDebug(BANJAX_PLUGIN_NAME, "exceeding excessive rate %f /msec", (*it)->rate);
          //clear the record to avoid multiple reporting to swabber
          //we are not clearing the state cause it is not for sure that
          //swabber ban the ip due to possible failure of acquiring lock
          // cur_ip_state.detail.begin_msec = 0;
          // cur_ip_state.detail.rate = 0;
          // ip_database->set_ip_state(ip, REGEX_BANNER_FILTER_ID, cur_ip_state.state_allocator);
          return make_pair(REGEX_MATCHED, (*it));
        }
      }
  }

  //no match
  return make_pair(REGEX_MISSED, (RatedRegex*)NULL);

}

FilterResponse RegexManager::execute(const TransactionParts& transaction_parts)
{
  const string sep(" ");
  TransactionParts ats_record_parts = (TransactionParts) transaction_parts;

  string ats_record =  ats_record_parts[TransactionMuncher::METHOD] + sep;
  ats_record+= ats_record_parts[TransactionMuncher::URL] + sep;
  ats_record+= ats_record_parts[TransactionMuncher::HOST] + sep;
  ats_record+= ats_record_parts[TransactionMuncher::UA];

  TSDebug(BANJAX_PLUGIN_NAME, "Examining %s for banned matches", ats_record.c_str());
  pair<RegexResult,RatedRegex*> result = parse_request(
						ats_record_parts[TransactionMuncher::IP],
						ats_record,
						ats_record_parts[TransactionMuncher::METHOD]
						);
  if (result.first == REGEX_MATCHED) {
    TSDebug(BANJAX_PLUGIN_NAME, "asking swabber to ban client ip: %s", ats_record_parts[TransactionMuncher::IP].c_str());
    
    //here instead we are calling nosmos's banning client
    string banning_reason = "matched regex rule " + result.second->rule_name;
    swabber_interface->ban(ats_record_parts[TransactionMuncher::IP], banning_reason);
    return FilterResponse(static_cast<ResponseGenerator>(&RegexManager::generate_response));

  } else if (result.first != REGEX_MISSED) {
    TSError("Regex failed with error: %d\n", result.first);
  }

  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
                    
}

std::string RegexManager::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{
  (void)transaction_parts; (void)response_info;
  char* forbidden_response = new char[forbidden_message_length+1];
  memcpy((void*)forbidden_response, (const void*)forbidden_message.c_str(), (forbidden_message_length+1)*sizeof(char));
  return forbidden_response;
}
