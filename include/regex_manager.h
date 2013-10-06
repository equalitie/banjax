/*
*  Part of regex_ban plugin: read the set of regexes and applying them
*
*  Vmon: May 2013: Initial version.
*/
#ifndef REGEX_MANAGER_H
#define REGEX_MANAGER_H

#include "banjax_filter.h"
#include "swabber_interface.h"

class RegexManager : public BanjaxFilter
{
 protected:
  //list of compiled banning_regex, called for matching everytime
  //the filter get a new connection
  //the idea is that the regex can add stuff at the end
  list<RE2*> banning_regexes;

  //swabber object used for banning bots
  SwabberInterface swabber_interface;  
  /**
    applies all regex to an ATS record

    @param ats_record: the full request record including time url agent etc
    @return: 1 match 0 not match < 0 error.
  */

 public:
  enum RegexResult{
    REGEX_MISSED,
    REGEX_MATCHED
  };
  enum RegexResult parse_request(string ats_record);
  /**
     receives the db object need to read the regex list,
     subsequently it reads all the regexs

  */
 RegexManager(const std::string& banjax_dir, const libconfig::Setting& main_root)
   :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, REGEX_BANNER_FILTER_ID, REGEX_BANNER_FILTER_NAME)
  {
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
      TransactionMuncher::IP   |
      TransactionMuncher::URL  |
      TransactionMuncher::HOST |
      TransactionMuncher::UA;}    

  /**
     overloaded execute to execute the filter, it assemble the
     parts to make ats record and then call the parse log
  */
  FilterResponse execute(const TransactionParts& transaction_parts);

  virtual std::string generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info);

};
  
#endif /* regex_manager.h */
