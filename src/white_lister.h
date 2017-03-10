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
  // List of previleged IPs that don't need to go through
  // other filters.
  std::list<SubnetRange> white_list;

public:
  /**
     receives the config object need to read the ip list,
     subsequently it reads all the ips
  */
  WhiteLister(const std::string& banjax_dir, const FilterConfig& filter_config) :
    BanjaxFilter::BanjaxFilter(banjax_dir, filter_config, WHITE_LISTER_FILTER_ID, WHITE_LISTER_FILTER_NAME)
  {
    queued_tasks[HTTP_REQUEST] = this;
    load_config();
  }

  uint64_t requested_info() override {
    return TransactionMuncher::IP;
  }

  FilterResponse on_http_request(const TransactionParts& transaction_parts) override;
  void on_http_close(const TransactionParts& transaction_parts) override {};

private:
  void load_config();
};

#endif /* white_lister.h */
