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
#include <ts/ts.h>

#define protected public
#define private public

#include "banjax.h"
#include "challenger.h"

using namespace std;
using MW = HostChallengeSpec::MagicWord;

static string TEMP_DIR = "/tmp";

BOOST_AUTO_TEST_SUITE(ChallengerUnitTests)

unique_ptr<Challenger> open_config(const std::string& config)
{
  YAML::Node cfg = YAML::Load(config);

  FilterConfig filter_config;

  for(auto i = cfg.begin(); i != cfg.end(); ++i) {
    std::string node_name = i->first.as<std::string>();

    if (node_name == "challenger")
      filter_config.config_node_list.push_back(i);
  }

  return unique_ptr<Challenger>(
      new Challenger(TEMP_DIR, filter_config, nullptr, nullptr, nullptr));
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
  auto challenges = mgr->host_challenges_static.at("example.co");
  BOOST_CHECK_EQUAL(challenges.size(), 1u);

  set<MW> expected({MW::make_substr("old_style_back_compat")});

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
  auto challenges = mgr->host_challenges_static.at("example.co");
  BOOST_CHECK_EQUAL(challenges.size(), 1u);

  set<MW> expected({MW::make_substr("wp-admin"),
                    MW::make_substr("wp-login.php")});

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
  auto challenges = mgr->host_challenges_static.at("example.co");
  BOOST_CHECK_EQUAL(challenges.size(), 1u);

  set<MW> expected({MW::make_substr("wp-admin"),
                    MW::make_substr("wp-login.php")});

  auto magic_words = challenges.front()->magic_words;

  BOOST_CHECK(magic_words == expected);
}

BOOST_AUTO_TEST_CASE(challenger_magic_word_mixed) {
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
    "        - 'wp-login0.php'\n"
    "        - ['substr', 'wp-login1.php']\n"
    "        - ['regexp', 'wp-login2.php']\n"
    "      magic_word_exceptions:\n"
    "        - \"wp-admin/admin.ajax.php\" \n"
    "        - \"wp-admin/another_script.php\" \n"
    "      validity_period: 360000\n"
    "      no_of_fails_to_ban: 10\n";

  auto mgr = open_config(config);
  auto challenges = mgr->host_challenges_static.at("example.co");
  BOOST_CHECK_EQUAL(challenges.size(), 1u);

  set<MW> expected({MW::make_substr("wp-login0.php"),
                    MW::make_substr("wp-login1.php"),
                    MW::make_regexp("wp-login2.php")});

  auto magic_words = challenges.front()->magic_words;

  BOOST_CHECK(magic_words == expected);
}

BOOST_AUTO_TEST_SUITE_END()
