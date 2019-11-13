/*
 * WhiteLister unit test set
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: Sept 2013, Initial version
 */

#include <boost/test/unit_test.hpp>
#include <string>
#include <yaml-cpp/yaml.h>
#include <stdio.h>
#include <ts/ts.h>

#include "banjax.h"
#include "white_lister.h"

BOOST_AUTO_TEST_SUITE(WhiteListerManagerUnitTests)
using namespace std;

static std::unique_ptr<BanjaxFilter> open_config(GlobalWhiteList& db, std::string config)
{
  YAML::Node cfg = YAML::Load(config);

  FilterConfig filter_config;

  try {
    for(auto i = cfg.begin(); i != cfg.end(); ++i) {
      std::string node_name = i->first.as<std::string>();
      if (node_name == "white_lister")
        filter_config.config_node_list.push_back(i);
    }
  }
  catch(YAML::RepresentationException& e)
  {
    BOOST_REQUIRE(false);
  }

  return unique_ptr<WhiteLister>(new WhiteLister(filter_config, db));
}

/**
   make up a fake transaction and check that the ip get white listed
 */
BOOST_AUTO_TEST_CASE(white_listed_ip)
{
  GlobalWhiteList db;
  auto test = open_config(db,
                          "white_lister:\n"
                          "  white_listed_ips: \n"
                          "    - 127.0.0.1\n");

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "127.0.0.1";
  mock_transaction[TransactionMuncher::HOST] = "localhost";

  FilterResponse cur_filter_result = test->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::SERVE_IMMIDIATELY_DONT_CACHE);
}

BOOST_AUTO_TEST_CASE(white_listed_ip2)
{
  GlobalWhiteList db;
  auto test = open_config(db,
                          "white_lister:\n"
                          "  white_listed_ips: \n"
                          "    - 11.22.33.44\n"
                          "    - 127.0.0.1\n");

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "127.0.0.1";
  mock_transaction[TransactionMuncher::HOST] = "localhost";

  FilterResponse cur_filter_result = test->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::SERVE_IMMIDIATELY_DONT_CACHE);
}

/**
   make up a fake transaction and check that an ip get doesn't get white listed
 */
BOOST_AUTO_TEST_CASE(ordinary_ip)
{
  GlobalWhiteList db;
  auto test = open_config(db,
                          "white_lister:\n"
                          "  white_listed_ips: \n"
                          "    - x.y.y.z\n"
                          "    - 127.0.0.1\n");

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "123.124.125.126";
  mock_transaction[TransactionMuncher::HOST] = "localhost";

  FilterResponse cur_filter_result = test->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::GO_AHEAD_NO_COMMENT);
}

/**
   make up a fake transaction and check that the ip get white listed even if it is an
   invalid one
 */
BOOST_AUTO_TEST_CASE(invalid_white_ip)
{
  GlobalWhiteList db;
  auto test = open_config(db,
                          "white_lister:\n"
                          "  white_listed_ips: \n"
                          "    - x.y.y.z\n"
                          "    - 127.0.0.1\n");

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "x.y.z.w";
  mock_transaction[TransactionMuncher::HOST] = "localhost";

  FilterResponse cur_filter_result = test->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::SERVE_IMMIDIATELY_DONT_CACHE);
}

BOOST_AUTO_TEST_SUITE_END()
