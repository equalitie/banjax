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

#define protected public

#include "util.h"
#include "banjax.h"
#include "unittest_common.h"
#include "challenge_manager.h"

using namespace std;

static string TEMP_DIR = "/tmp";

BOOST_AUTO_TEST_SUITE(ChallengeManagerUnitTests)

unique_ptr<ChallengeManager> open_config(const std::string& config)
{
  YAML::Node cfg = YAML::Load(config);

  FilterConfig filter_config;

  for(auto i = cfg.begin(); i != cfg.end(); ++i) {
    std::string node_name = i->first.as<std::string>();

    if (node_name == "challenger")
      filter_config.config_node_list.push_back(i);
  }

  return unique_ptr<ChallengeManager>(
      new ChallengeManager(TEMP_DIR, filter_config, nullptr, nullptr));
}

BOOST_AUTO_TEST_CASE(load_config1) {
  std::string config =
    "challenger:\n"
    "  difficulty: 0 \n"
    "  key: \"foobar-key\" \n"
    "  challenges:\n"
    "    - name: \"example.co_auth\" \n"
    "      domains:\n"
    "       - \"example.co\" \n"
    "       - \"www.example.co\" \n"
    "      challenge_type: \"auth\" \n"
    "      challenge: \"auth.html\" \n"
    "      password_hash: \"BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM=\" \n"
    "      # sha256(\"howisbabbyformed?\")\n"
    "      magic_word: old_style_back_compat\n"
    "      magic_word_exceptions:\n"
    "        - \"wp-admin/admin.ajax.php\" \n"
    "        - \"wp-admin/another_script.php\" \n"
    "      validity_period: 360000\n"
    "      no_of_fails_to_ban: 10\n";

  auto mgr = open_config(config);
  auto challenges = mgr->host_challenges.at("example.co");
  BOOST_CHECK_EQUAL(challenges.size(), 1);

  set<string> expected({"old_style_back_compat"});

  auto magic_words = challenges.front()->magic_words;

  BOOST_CHECK(magic_words == expected);
}

BOOST_AUTO_TEST_CASE(load_config2) {
  std::string config =
    "challenger:\n"
    "  difficulty: 0 \n"
    "  key: \"foobar-key\" \n"
    "  challenges:\n"
    "    - name: \"example.co_auth\" \n"
    "      domains:\n"
    "       - \"example.co\" \n"
    "       - \"www.example.co\" \n"
    "      challenge_type: \"auth\" \n"
    "      challenge: \"auth.html\" \n"
    "      password_hash: \"BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM=\" \n"
    "      # sha256(\"howisbabbyformed?\")\n"
    "      magic_word:\n"
    "        - \"wp-admin\" \n"
    "        - \"wp-login.php\" \n"
    "      magic_word_exceptions:\n"
    "        - \"wp-admin/admin.ajax.php\" \n"
    "        - \"wp-admin/another_script.php\" \n"
    "      validity_period: 360000\n"
    "      no_of_fails_to_ban: 10\n";

  auto mgr = open_config(config);
  auto challenges = mgr->host_challenges.at("example.co");
  BOOST_CHECK_EQUAL(challenges.size(), 1);

  set<string> expected({"wp-admin", "wp-login.php"});

  auto magic_words = challenges.front()->magic_words;

  BOOST_CHECK(magic_words == expected);
}

BOOST_AUTO_TEST_CASE(load_config3) {
  std::string config =
    "challenger:\n"
    "  difficulty: 0 \n"
    "  key: \"foobar-key\" \n"
    "  challenges:\n"
    "    - name: \"example.co_auth\" \n"
    "      domains:\n"
    "       - \"example.co\" \n"
    "       - \"www.example.co\" \n"
    "      challenge_type: \"auth\" \n"
    "      challenge: \"auth.html\" \n"
    "      password_hash: \"BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM=\" \n"
    "      # sha256(\"howisbabbyformed?\")\n"
    "      magic_word: [\"wp-admin\", \"wp-login.php\"] \n"
    "      magic_word_exceptions:\n"
    "        - \"wp-admin/admin.ajax.php\" \n"
    "        - \"wp-admin/another_script.php\" \n"
    "      validity_period: 360000\n"
    "      no_of_fails_to_ban: 10\n";

  auto mgr = open_config(config);
  auto challenges = mgr->host_challenges.at("example.co");
  BOOST_CHECK_EQUAL(challenges.size(), 1);

  set<string> expected({"wp-admin", "wp-login.php"});

  auto magic_words = challenges.front()->magic_words;

  BOOST_CHECK(magic_words == expected);
}

BOOST_AUTO_TEST_SUITE_END()
