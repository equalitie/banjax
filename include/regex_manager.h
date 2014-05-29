/*
*  Part of regex_ban plugin: read the set of regexes and applying them
*
*  Vmon: May 2013: Initial version.
*/
#ifndef REGEX_MANAGER_H
#define REGEX_MANAGER_H

#include <utility> //for pair

#include "banjax_filter.h"
#include "swabber_interface.h"

struct RatedRegex
{
  std::string rule_name;
  RE2* re2_regex;
  unsigned int interval; //interval to look in mseconds
  float rate; //threshold /interval

  RatedRegex(std::string new_rule_name, RE2* regex, unsigned int observation_interval, float excessive_rate):
    rule_name(new_rule_name),
    re2_regex(regex),
    interval(observation_interval),
    rate(excessive_rate) {}

};

struct RegexBannerState {
  unsigned long begin_msec;
  float rate;
};

union RegexBannerStateUnion
{
  FilterState state_allocator;
  RegexBannerState detail;

  RegexBannerStateUnion()
    : state_allocator() { }
  
};

class RegexManager : public BanjaxFilter
{
 protected:
  //We store the forbidden message at the begining so we can copy it fast 
  //everytime. It is being stored here for being used again
  //ATS barf if you give it the message in const memory
  const std::string forbidden_message;
  const size_t forbidden_message_length;

  //list of compiled banning_regex, called for matching everytime
  //the filter get a new connection
  //the idea is that the regex can add stuff at the end
  std::list<RatedRegex*> rated_banning_regexes;

  //swabber object used for banning bots
  SwabberInterface* swabber_interface;

 public:
  enum RegexResult{
    REGEX_MISSED,
    REGEX_MATCHED
  };
  /**
    applies all regex to an ATS record

    @param ats_record: the full request record including time url agent etc
    @return: pair of 1 match 0 not match < 0 error and
             the matched regex (for loging) or NULL in miss
  */

  std::pair<RegexResult,RatedRegex*> parse_request(std::string ip, std::string ats_record, std::string method);
  /**
     receives the db object need to read the regex list,
     subsequently it reads all the regexs

  */
 RegexManager(const std::string& banjax_dir, const libconfig::Setting& main_root, IPDatabase* global_ip_database, SwabberInterface* global_swabber_interface)
   :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, REGEX_BANNER_FILTER_ID, REGEX_BANNER_FILTER_NAME),
    forbidden_message("<html><header></header><body>Forbidden</body></html>"),
    forbidden_message_length(forbidden_message.length()),
    swabber_interface(global_swabber_interface)
  {
    queued_tasks[HTTP_REQUEST] = static_cast<FilterTaskFunction>(&RegexManager::execute);
    ip_database = global_ip_database;
    load_config(main_root[BANJAX_FILTER_NAME]);
  }

  /**
    Overload of the load config
    reads all the regular expressions from the database.
    and compile them
  */
  virtual void load_config(libconfig::Setting& cfg);

  /**
     Overloaded to tell banjax that we need url, host, ua and ip
     for banning
     At this point we only asks for url, host and user agent
     later we can ask more if it is needed
   */
  uint64_t requested_info() { return 
      TransactionMuncher::IP     |
      TransactionMuncher::METHOD |
      TransactionMuncher::URL    |
      TransactionMuncher::HOST   |
      TransactionMuncher::UA;}    

  /**
     overloaded execute to execute the filter, it assemble the
     parts to make ats record and then call the parse log
  */
  FilterResponse execute(const TransactionParts& transaction_parts);

  virtual std::string generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info);

};
  
#endif /* regex_manager.h */
