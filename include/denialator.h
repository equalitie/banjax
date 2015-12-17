/*
*  A subfilter of banjax that deny access to any
*  bot which has been reported to swabber. it reply a message
*
*  Vmon: Dec 2015: Initial version.
*/
#ifndef DENIALATOR_H
#define DENIALATOR_H

#include <yaml-cpp/yaml.h>
#include "banjax_filter.h"

class Denialtor : public BanjaxFilter
{
 protected:

 public:
  /**
     receives the config object need to read the ip list,
     subsequently it reads all the ips

  */
 Denialtor(const std::string& banjax_dir, const FilterConfig& filter_config)
   :BanjaxFilter::BanjaxFilter(banjax_dir, filter_config, WHITE_LISTER_FILTER_ID, WHITE_LISTER_FILTER_NAME)
  {
    queued_tasks[HTTP_REQUEST] = static_cast<FilterTaskFunction>(&WhiteLister::execute);
    load_config();
  }

  /**
    Overload of the load config
    reads all the regular expressions from the database.
    and compile them
  */
  virtual void load_config();

  /**
     Overloaded to tell banjax that we need url, host, ua and ip
     for banning
     At this point we only asks for the ip
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
