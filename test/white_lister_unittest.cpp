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

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <zmq.hpp>

#include <re2/re2.h> //google re2

#include <yaml-cpp/yaml.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtest/gtest.h> //google test

#include <ts/ts.h>

#include "util.h"
#include "banjax.h"
#include "unittest_common.h"
#include "white_lister.h"

using namespace std;

/**
   Mainly fill an string stream buffer with a predefined configuration
   and to check if the white lister has picked them correctly and
   match them correctly.
 */
class WhiteListerTest : public testing::Test {
 protected:

  YAML::Node cfg;
  string TEMP_DIR = "/tmp";

  std::unique_ptr<BanjaxFilter> test_white_lister;

  void open_config(std::string config)
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
      ASSERT_TRUE(false);
    }

    test_white_lister.reset(new WhiteLister(TEMP_DIR, filter_config));
  }
};

/**
   make up a fake transaction and check that the ip get white listed
 */
TEST_F(WhiteListerTest, white_listed_ip)
{
  open_config("white_lister:\n"
              "  white_listed_ips: \n"
              "    - 127.0.0.1\n");

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "127.0.0.1";

  FilterResponse cur_filter_result = test_white_lister->execute(mock_transaction);

  EXPECT_EQ(cur_filter_result.response_type, FilterResponse::NO_WORRIES_SERVE_IMMIDIATELY);
}

TEST_F(WhiteListerTest, white_listed_ip2)
{
  open_config("white_lister:\n"
              "  white_listed_ips: \n"
              "    - 11.22.33.44\n"
              "    - 127.0.0.1\n");

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "127.0.0.1";

  FilterResponse cur_filter_result = test_white_lister->execute(mock_transaction);

  EXPECT_EQ(cur_filter_result.response_type, FilterResponse::NO_WORRIES_SERVE_IMMIDIATELY);
}

/**
   make up a fake transaction and check that an ip get doesn't get white listed
 */
TEST_F(WhiteListerTest, ordinary_ip)
{
  open_config("white_lister:\n"
              "  white_listed_ips: \n"
              "    - x.y.y.z\n"
              "    - 127.0.0.1\n");

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "123.124.125.126";

  FilterResponse cur_filter_result = test_white_lister->execute(mock_transaction);

  EXPECT_EQ(cur_filter_result.response_type, FilterResponse::GO_AHEAD_NO_COMMENT);
}

/**
   make up a fake transaction and check that the ip get white listed even if it is an
   invalid one
 */
TEST_F(WhiteListerTest, invalid_white_ip)
{
  open_config("white_lister:\n"
              "  white_listed_ips: \n"
              "    - x.y.y.z\n"
              "    - 127.0.0.1\n");

  //first we make a mock up request
  TransactionParts mock_transaction;
  mock_transaction[TransactionMuncher::IP] = "x.y.z.w";

  FilterResponse cur_filter_result = test_white_lister->execute(mock_transaction);

  EXPECT_EQ(cur_filter_result.response_type, FilterResponse::NO_WORRIES_SERVE_IMMIDIATELY);
}

