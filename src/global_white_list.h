#pragma once

#include <boost/optional.hpp>
#include "util.h"

class GlobalWhiteList {
public:

  void insert(const SubnetRange& ip_range) {
      white_list.push_back(ip_range);
  }

  boost::optional<SubnetRange> is_white_listed(const std::string& ip) const
  {
    for (const auto& subnet_range : white_list) {
      if (is_match(ip, subnet_range)) {
        return subnet_range;
      }
    }
    return boost::none;
  }

private:
  std::list<SubnetRange> white_list;

};
