/*
*  A subfilter of banjax that lets administrative machines bypass
*  all filter with no inspection base on IP
*
*  Vmon: May 2013: Initial version.
*/
#ifndef WHITE_LISTER_H
#define WHITE_LISTER_H

#include <yaml-cpp/yaml.h>
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
  WhiteLister(const std::string& banjax_dir, const FilterConfig& filter_config) :
    BanjaxFilter::BanjaxFilter(banjax_dir, filter_config, WHITE_LISTER_FILTER_ID, WHITE_LISTER_FILTER_NAME)
  {
    load_config();
  }

  /**
     Overloaded to tell banjax that we need url, host, ua and ip
     for banning
     At this point we only asks for url, host and user agent
     later we can ask more if it is needed
   */
  uint64_t requested_info() override {
    return TransactionMuncher::IP;
  }

  /**
     overloaded execute to execute the filter, it assemble the
     parts to make ats record and then call the parse log
   */
  FilterResponse on_http_request(const TransactionParts& transaction_parts) override;
  void on_http_close(const TransactionParts& transaction_parts) override {};

private:
  void load_config();
};

#endif /* white_lister.h */
