/*
*  Part of regex_ban plugin: read the set of regexes and applying them
*
*  Vmon: May 2013: Initial version.
*/
#ifndef REGEX_MANAGER_H
#define REGEX_MANAGER_H

#include <vector>
#include <unordered_set>
#include <re2/re2.h>

#include "banjax_filter.h"
#include "swabber.h"

class RegexManager : public BanjaxFilter
{
private:
  struct RegexState {
    unsigned long begin_msec;
    float rate;
    RegexState() : begin_msec(0) , rate(0.0) {};
  };

  struct RatedRegex {
    size_t id;
    std::string rule_name;
    std::unique_ptr<RE2> re2_regex;
    unsigned int interval; //interval to look in mseconds
    float rate; //threshold /interval
    std::unordered_set<std::string> hosts_to_skip;
  
    RatedRegex(size_t new_id,
        std::string new_rule_name,
        RE2* regex,
        unsigned int observation_interval,
        float excessive_rate,
        std::vector<std::string> hosts_to_skip_in):
      id(new_id),
      rule_name(new_rule_name),
      re2_regex(regex),
      interval(observation_interval),
      rate(excessive_rate),
      hosts_to_skip(hosts_to_skip_in.begin(), hosts_to_skip_in.end()) {}
  };

public:
  using IpDb = ::IpDb<std::vector<RegexState>>;

protected:
  //We store the forbidden message at the begining so we can copy it fast
  //everytime. It is being stored here for being used again
  //ATS barf if you give it the message in const memory
  const std::string forbidden_message;
  const size_t forbidden_message_length;

  //list of compiled banning_regex, called for matching everytime
  //the filter get a new connection
  //the idea is that the regex can add stuff at the end
  std::list<std::unique_ptr<RatedRegex>> rated_banning_regexes;
  unsigned int total_no_of_rules;

  //swabber object used for banning bots
  Swabber* swabber;

  IpDb* regex_manager_ip_db;

protected:
  /**
    applies all regex to an ATS record
  */
  RatedRegex* try_match(std::string ip, std::string ats_record, std::string client_request_host) const;

public:
  /**
     receives the db object need to read the regex list,
     subsequently it reads all the regexs

  */
  RegexManager(const FilterConfig& filter_config,
               IpDb* regex_manager_ip_db,
               Swabber* swabber) :
    BanjaxFilter::BanjaxFilter(filter_config, REGEX_BANNER_FILTER_ID, REGEX_BANNER_FILTER_NAME),
    forbidden_message("<html><header></header><body>Forbidden</body></html>"),
    forbidden_message_length(forbidden_message.length()),
    swabber(swabber),
    regex_manager_ip_db(regex_manager_ip_db)
  {
    queued_tasks[HTTP_REQUEST] = this;
    load_config();
  }

  /**
     Overloaded to tell banjax that we need url, host, ua and ip
     for banning
     At this point we only asks for url, host and user agent
     later we can ask more if it is needed
   */
  uint64_t requested_info() {
    return TransactionMuncher::IP     |
           TransactionMuncher::METHOD |
           TransactionMuncher::URL    |
           TransactionMuncher::HOST   |
           TransactionMuncher::UA;
  }

  FilterResponse on_http_request(const TransactionParts& transaction_parts) override;
  void on_http_close(const TransactionParts& transaction_parts) override {}

  std::string generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info) override;

private:
  void load_config();
};

#endif /* regex_manager.h */
