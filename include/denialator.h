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

class Denialator : public BanjaxFilter
{
 protected:
  const std::string forbidden_message;
  const size_t forbidden_message_length;

 public:
  /**
     receives the config object need to read the ip list,
     subsequently it reads all the ips

  */
 Denialator(const std::string& banjax_dir, const FilterConfig& filter_config, IPDatabase* global_ip_database)
   :BanjaxFilter::BanjaxFilter(banjax_dir, filter_config, DENIALATOR_FILTER_ID, DENIALATOR_FILTER_NAME),
    forbidden_message("<html><header></header><body>500 Internal Server Error</body></html>"),
    forbidden_message_length(forbidden_message.length())
  {
    queued_tasks[HTTP_REQUEST] = static_cast<FilterTaskFunction>(&Denialator::execute);
    ip_database = global_ip_database;
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
     we  overload generate_respons cause we have to say denied access
  */
  virtual std::string generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info);

};
  
#endif /* white_lister.h */
