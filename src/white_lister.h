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
#include "global_white_list.h"

class WhiteLister : public BanjaxFilter
{
protected:
  // List of privileged IPs.
  GlobalWhiteList& white_list;

public:
  /**
     receives the config object need to read the ip list,
     subsequently it reads all the ips
  */
  WhiteLister(const std::string& banjax_dir,
              const FilterConfig& filter_config,
              GlobalWhiteList& white_list) :
    BanjaxFilter::BanjaxFilter(banjax_dir, filter_config, WHITE_LISTER_FILTER_ID, WHITE_LISTER_FILTER_NAME),
    white_list(white_list)
  {
    queued_tasks[HTTP_REQUEST] = this;
    load_config();
  }

  uint64_t requested_info() override {
    return TransactionMuncher::IP |
           TransactionMuncher::HOST;
  }

  FilterResponse on_http_request(const TransactionParts& transaction_parts) override;
  void on_http_close(const TransactionParts& transaction_parts) override {};

private:
  void load_config();
};

#endif /* white_lister.h */
