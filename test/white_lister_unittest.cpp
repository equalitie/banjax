/*
 * WhiteLister unit test set
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: Sept 2013, Initial version
 */

// Copyright 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <zmq.hpp>

#include <re2/re2.h> //google re2

#include <yaml-cpp/yaml.h>
#include <stdio.h>
#include <stdlib.h>

#include <ts/ts.h>

#include "util.h"
#include "banjax.h"
#include "unittest_common.h"
#include "white_lister.h"
#include "global_white_list.h"

BOOST_AUTO_TEST_SUITE(WhiteListerManagerUnitTests)
using namespace std;

string TEMP_DIR = "/tmp";

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

  return unique_ptr<WhiteLister>(new WhiteLister(TEMP_DIR, filter_config, db));
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

  FilterResponse cur_filter_result = test->on_http_request(mock_transaction);

  BOOST_CHECK_EQUAL(cur_filter_result.response_type, FilterResponse::SERVE_IMMIDIATELY_DONT_CACHE);
}

BOOST_AUTO_TEST_SUITE_END()
