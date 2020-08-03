#include <boost/test/unit_test.hpp>
#include <string>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <ts/ts.h>
#include <memory>

#include "banjax.h"
#include "kafka.h"

BOOST_AUTO_TEST_SUITE(KafkaUnitTests)

/*
BOOST_AUTO_TEST_CASE(cold_start) {
  auto kp = std::make_unique<KafkaProducer>();
  BOOST_CHECK(kp != nullptr);

  std::string config_string =
    "kafka:\n"
    "  brokers: \"localhost:9092\" \n";

  YAML::Node config = YAML::Load(config_string);

  kp->load_config(config);
}

BOOST_AUTO_TEST_CASE(consumer_cold_start) {
  std::string config_string =
    "kafka:\n"
    "  brokers: \"localhost:9092\" \n";

  YAML::Node kafka_config = YAML::Load(config_string);

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

  YAML::Node cfg = YAML::Load(config);

  FilterConfig filter_config;

  for(auto i = cfg.begin(); i != cfg.end(); ++i) {
    std::string node_name = i->first.as<std::string>();

    if (node_name == "challenger")
      filter_config.config_node_list.push_back(i);
  }

  auto challenger = std::make_shared<Challenger>("/tmp/", filter_config, nullptr, nullptr, nullptr);

  auto kp = std::make_unique<KafkaConsumer>(kafka_config, challenger);
  BOOST_CHECK(kp != nullptr);
}
*/

BOOST_AUTO_TEST_SUITE_END()
