/**
   This is to keep the identification information of new filters as
   they being implemented 
 */

#ifndef FILTER_LIST_H
#define FILTER_LIST_H

#include <string>
#include <vector>

enum FilterIDType {
  REGEX_BANNER_FILTER_ID,
  CHALLENGER_FILTER_ID,
  BOT_SNIFFER_FILTER_ID,
  WHITE_LISTER_FILTER_ID,
  DENIALATOR_FILTER_ID,
  TOTAL_NO_OF_FILTERS
};

const std::string REGEX_BANNER_FILTER_NAME = "regex_banner";
const std::string CHALLENGER_FILTER_NAME = "challenger";
const std::string BOT_SNIFFER_FILTER_NAME = "bot_sniffer";
const std::string WHITE_LISTER_FILTER_NAME = "white_lister";
const std::string DENIALATOR_FILTER_NAME = "denialator";

const std::vector<std::string> all_filters_names = {
  REGEX_BANNER_FILTER_NAME,
  CHALLENGER_FILTER_NAME,
  BOT_SNIFFER_FILTER_NAME,
  WHITE_LISTER_FILTER_NAME,
  DENIALATOR_FILTER_NAME
};

#endif
