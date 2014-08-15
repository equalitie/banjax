/*
*  A subfilter of banjax that lets administrative machines bypass
*  all filter with no inspection base on IP
*
*  Vmon: May 2013: Initial version.
*/
#ifndef WHITE_LISTER_H
#define WHITE_LISTER_H

#include "banjax_filter.h"
#include "util.h"

class WhiteLister : public BanjaxFilter
{
 protected:
  //list of previlaged ips that don't need to go through
  //the banjax filtering process
  std::list<SubnetRange> white_list;

 public:
  /**
     receives the config object need to read the ip list,
     subsequently it reads all the ips

  */
 WhiteLister(const std::string& banjax_dir, const libconfig::Setting& main_root)
   :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, WHITE_LISTER_FILTER_ID, WHITE_LISTER_FILTER_NAME)
  {
    queued_tasks[HTTP_REQUEST] = static_cast<FilterTaskFunction>(&WhiteLister::execute);
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
      TransactionMuncher::IP;}    

  /**
     overloaded execute to execute the filter, it assemble the
     parts to make ats record and then call the parse log
   */
  FilterResponse execute(const TransactionParts& transaction_parts);

  /**
     we do not overload generate_respons cause we have no response to generate
  */

};
  
#endif /* white_lister.h */
