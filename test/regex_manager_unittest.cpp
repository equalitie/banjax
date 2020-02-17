/*
 * RegexManager unit test set
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 */

#include <boost/test/unit_test.hpp>
#include <string>
#include <yaml-cpp/yaml.h>
#include <stdio.h>
#include <stdlib.h>
#include <ts/ts.h>

#include "banjax.h"
#include "unittest_common.h"
#include "regex_manager.h"
#include "swabber.h"
#include "ip_db.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(RegexManagerUnitTests)

static std::string default_config() {
  return
    "regex_banner:\n"
    "  - rule: simple to ban\n"
    "    regex: '.*simple_to_ban.*'\n"
    "    interval: 1\n"
    "    hits_per_interval: 0\n"
    "    hosts_to_skip: ['host-with-skip-rule.com'] \n"
    "  - rule: hard to ban\n"
    "    regex: '.*not%20so%20simple%20to%20ban[\\s\\S]*'\n"
    "    interval: 1\n"
    "    hits_per_interval: 0\n"
    "  - rule: 'flooding ban' \n"
    "    regex: '.*flooding_ban.*'\n"
    "    interval: 30 \n"
    "    hits_per_interval: 10\n"
    "  - rule: 'flooding ban 2' \n"
    "    regex: '.*flooding_diff_ban.*'\n"
    "    interval: 30 \n"
    "    hits_per_interval: 10\n";
}

/**
   Mainly fill an string stream buffer with a predefined configuration
   and to check if the regex manager has picked them correctly and
   match them correctly.
 */
class Test {
public:
  std::unique_ptr<BanjaxFilter> regex_manager;

  Swabber::IpDb swabber_ip_db;
  RegexManager::IpDb regex_manager_ip_db;
  Swabber test_swabber;

  Test(std::string config = default_config())
    :  test_swabber(&swabber_ip_db)
  {
    open_config(move(config));
  }

private:
  void open_config(std::string config)
  {
    YAML::Node cfg = YAML::Load(config);

    FilterConfig regex_filter_config;

    try {
      for(auto i = cfg.begin(); i != cfg.end(); ++i) {
        std::string node_name = i->first.as<std::string>();
        if (node_name == "regex_banner")
          regex_filter_config.config_node_list.push_back(i);
      }
    }
    catch(YAML::RepresentationException& e)
    {
      BOOST_REQUIRE(false);
    }

    regex_manager.reset(
        new RegexManager(regex_filter_config,
                         &regex_manager_ip_db,
                         &test_swabber));
  }
};

/**
   read a pre determined config file and check if the values are
   as expected for the regex manager
 */
BOOST_AUTO_TEST_CASE(load_config) {
  // Avoid warnings that there is no check in this test as we're checking
  // whether the initialization dies or not.
  BOOST_CHECK(true);
  Test test;
}

/**
   make up a fake GET request and check that the manager is banning
 */
BOOST_AUTO_TEST_CASE(match)
{
  Test test;

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::METHOD] = "GET";
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";
  mock_transaction[TransactionMuncher::URL] = "http://simple_to_ban_me/";
  mock_transaction[TransactionMuncher::HOST] = "neverhood.com";
  mock_transaction[TransactionMuncher::UA] = "neverhood browsing and co";

  FilterResponse cur_filter_result = test.regex_manager->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::I_RESPOND);
}

/**
 * A test to fix a github issue:
 * https://github.com/equalitie/banjax/issues/35
 */
BOOST_AUTO_TEST_CASE(match_blank)
{
  auto config =
    "regex_banner:\n"
    "  - rule: simple to ban\n"
    "    regex: '^GET\\ .*mywebsite\\.org\\ $'\n"
    "    interval: 1\n"
    "    hits_per_interval: 0\n";

  Test test(config);

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::METHOD] = "GET";
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";
  mock_transaction[TransactionMuncher::URL] = "http:///";
  mock_transaction[TransactionMuncher::HOST] = "mywebsite.org";
  mock_transaction[TransactionMuncher::UA] = "";

  FilterResponse cur_filter_result = test.regex_manager->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::I_RESPOND);
}

/**
   make up a fake GET request and check that the manager is not banning
 */
BOOST_AUTO_TEST_CASE(miss)
{
  Test test;

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::METHOD] = "GET";
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";
  mock_transaction[TransactionMuncher::URL] = "http://dont_ban_me/";
  mock_transaction[TransactionMuncher::HOST] = "neverhood.com";
  mock_transaction[TransactionMuncher::UA] = "neverhood browsing and co";

  FilterResponse cur_filter_result = test.regex_manager->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::GO_AHEAD_NO_COMMENT);
}


/**
   make up a fake GET request and check that the manager is not banning when
   the request method changes to POST despite passing the allowed rate
 */
BOOST_AUTO_TEST_CASE(post_get_counter)
{
  Test test;

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::METHOD] = "GET";
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";
  mock_transaction[TransactionMuncher::URL] = "http://flooding_ban/";
  mock_transaction[TransactionMuncher::HOST] = "neverhood.com";
  mock_transaction[TransactionMuncher::UA] = "neverhood browsing and co";
  FilterResponse cur_filter_result = FilterResponse::GO_AHEAD_NO_COMMENT;

  for ( int i=0; i<2; i++ ) {
    cur_filter_result = test.regex_manager->on_http_request(mock_transaction);
  }

  mock_transaction[TransactionMuncher::URL] = "http://flooding_diff_ban/";
  cur_filter_result = test.regex_manager->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::GO_AHEAD_NO_COMMENT);
}

/**
   make up a fake GET request and check that the manager is banning a request
   with special chars that . doesn't match
 */
BOOST_AUTO_TEST_CASE(match_special_chars)
{
  Test test;

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::METHOD] = "GET";
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";
  mock_transaction[TransactionMuncher::URL] = "http://not%20so%20simple%20to%20ban//";
  mock_transaction[TransactionMuncher::HOST] = "neverhood.com";
  mock_transaction[TransactionMuncher::UA] = "\"[this is no simple]\" () * ... :; neverhood browsing and co";

  FilterResponse cur_filter_result = test.regex_manager->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::I_RESPOND);
}

/**
   Check the response
 */
BOOST_AUTO_TEST_CASE(forbidden_response)
{
  Test test;

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::METHOD] = "GET";
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";
  mock_transaction[TransactionMuncher::URL] = "http://simple_to_ban_me/";
  mock_transaction[TransactionMuncher::HOST] = "neverhood.com";
  mock_transaction[TransactionMuncher::UA] = "neverhood browsing and co";

  FilterResponse cur_filter_result = test.regex_manager->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::I_RESPOND);

  BOOST_CHECK_EQUAL("<html><header></header><body>Forbidden</body></html>", test.regex_manager->generate_response(mock_transaction, cur_filter_result));
}

BOOST_AUTO_TEST_CASE(domain_with_skip_rule)
{
  Test test;

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::METHOD] = "GET";
  mock_transaction[TransactionMuncher::IP] = "123.456.789.123";
  mock_transaction[TransactionMuncher::URL] = "http://simple_to_ban/";
  mock_transaction[TransactionMuncher::HOST] = "host-with-skip-rule.com";
  mock_transaction[TransactionMuncher::UA] = "ua";

  FilterResponse cur_filter_result = test.regex_manager->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::GO_AHEAD_NO_COMMENT);
}

//TOOD: We need a test that listen on the publication and see if the ip really being send on the port

BOOST_AUTO_TEST_SUITE_END()
