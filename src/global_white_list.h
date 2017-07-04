#pragma once

#include <boost/optional.hpp>
#include "util.h"

class GlobalWhiteList {
public:

  void insert(const SubnetRange& ip_range) {
    white_list.push_back(ip_range);
  }

  void insert(const std::string& host, const SubnetRange& ip_range) {
    per_host_white_list[host].insert(ip_range);
  }

  boost::optional<SubnetRange> is_white_listed(const std::string& ip) const
  {
    for (const auto& r : white_list) {
      if (is_match(ip, r)) {
        return r;
      }
    }

    return boost::none;
  }

  boost::optional<SubnetRange> is_white_listed(const std::string& host, const std::string& ip) const
  {
    if (auto r = is_white_listed(ip)) {
      return r;
    }

    auto i = per_host_white_list.find(host);

    if (i == per_host_white_list.end()) {
      return boost::none;
    }

    for (const auto& r : i->second) {
      if (is_match(ip, r)) {
        return r;
      }
    }

    return boost::none;
  }

private:
  std::map<std::string, std::set<SubnetRange>> per_host_white_list;
  std::list<SubnetRange> white_list;

};
