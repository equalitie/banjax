#include "kafka.h"
#include <iostream>
#include <librdkafka/rdkafkacpp.h>

KafkaProducer::KafkaProducer() {
  print::debug("KafkaProducer default constructor");
}

KafkaProducer::KafkaProducer(
    std::unique_ptr<RdKafka::Producer> old_rdk_producer,
    FilterConfig &old_filter_config) {
  if (old_rdk_producer) {
    old_rdk_producer = std::move(old_rdk_producer);
  }
}

void KafkaProducer::load_config(YAML::Node &new_config) {
  if (new_config.Type() != YAML::NodeType::Map) {
      print::debug("KafkaProducer::load_config requires a YAML::Map");
    throw;
  }

  if (new_config == old_config) {
    current_rdk_producer = std::move(old_rdk_producer);
    return;
  }

  auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

  auto brokers = new_config["kafka"]["brokers"].as<std::string>();
  std::string errstr;
  if (conf->set("metadata.broker.list", brokers, errstr) !=
      RdKafka::Conf::CONF_OK) {
      print::debug("KafkaProducer: bad 'brokers' config: ", errstr);
    throw;
  }

  current_rdk_producer.reset(RdKafka::Producer::create(conf, errstr));
  if (!current_rdk_producer) {
      print::debug("KafkaProducer: failed to create Producer");
    throw;
  }

  const std::string topic_str = "test";
  std::string line_value = "hello world";

  for (std::string line; std::getline(std::cin, line_value);) {
    RdKafka::ErrorCode err = current_rdk_producer->produce(
        /* Topic name */
        topic_str,
        /* Any Partition: the builtin partitioner will be
                * used to assign the message to a topic based
                * on the message key, or random partition if
                * the key is not set. */
        (int)RdKafka::Topic::PARTITION_UA,
        /* Make a copy of the value */
        (int)RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
        /* Value */
        (void *)(line_value.c_str()), (size_t)line_value.size(),
        /* Key */
        NULL, (size_t)0,
        /* Timestamp (defaults to current time) */
        0,
        /* Message headers, if any */
        NULL);
    if (err == RdKafka::ERR_NO_ERROR) {
        print::debug("produce() succeeded");
    } else {
        print::debug(": Failed to produce()");
    }
    if (line_value == "next") {
        break;
    }
  }
  print::debug("producer exiting");
}

void
KafkaConsumer::reload_config(YAML::Node& config, std::shared_ptr<Challenger> challenger) {
    TSMutexLock(stored_config_lock);
    auto on_scope_exit = defer([&] { TSMutexUnlock(stored_config_lock); });
    stored_config = config;
    stored_challenger = challenger;
    config_valid = false;
}


KafkaConsumer::KafkaConsumer(YAML::Node &new_config, std::shared_ptr<Challenger> challenger)
  : stored_config_lock(TSMutexCreate()),
    stored_config(new_config),
    stored_challenger(challenger)
{
    thread_handle = std::thread([=] {
        print::debug("hello from lambda");
        while (!shutting_down) {
            print::debug("(RE)LOADING CONFIG");
            auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
            auto tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
            std::string errstr;
            {
                TSMutexLock(stored_config_lock);
                auto on_scope_exit = defer([&] { TSMutexUnlock(stored_config_lock); });
                if (stored_config.Type() != YAML::NodeType::Map) {
                print::debug("KafkaConsumer::load_config requires a YAML::Map");
                    throw;
                }
        
                auto brokers = stored_config["kafka"]["brokers"].as<std::string>();
                if (conf->set("metadata.broker.list", brokers, errstr) !=
                    RdKafka::Conf::CONF_OK) {
                    print::debug("KafkaConsumer: bad 'brokers' config: ", errstr);
                    throw;
                }
                if (conf->set("group.id", "285723", errstr) !=
                    RdKafka::Conf::CONF_OK) {
                    print::debug("KafkaConsumer: bad group.id config: ", errstr);
                    throw;
                }
                config_valid = true;
            }
          
            RdKafka::KafkaConsumer *consumer = RdKafka::KafkaConsumer::create(conf, errstr);
            if (!consumer) {
                print::debug("Failed to create consumer: ", errstr);
                throw;
            }
          
            print::debug("% Created consumer ", consumer->name());
          
            std::vector<std::string> topics;
            topics.push_back("test");
            RdKafka::ErrorCode resp = consumer->subscribe(topics);
            if (resp != RdKafka::ERR_NO_ERROR) {
                print::debug("Failed to start consumer: ", RdKafka::err2str(resp));
                throw; // XXX inside this thread?...
            }

            while (config_valid && !shutting_down) {
                std::cerr << "BLOCKING" << std::endl;
                RdKafka::Message *msg = consumer->consume(2000);
                msg_consume(msg, NULL, stored_challenger);
                delete msg;
                stored_challenger->remove_expired_challenges();
            }

            print::debug("BEFORE CLOSE");
            consumer->close();
            print::debug("AFTER CLOSE");
        }
        print::debug("THREAD EXITING");
    });
    print::debug("hello from OUTSIDE lambda");
}


void msg_consume(RdKafka::Message *message, void *opaque, std::shared_ptr<Challenger> challenger) {
    print::debug("MSG_CONSUME()");
  switch (message->err()) {
  case RdKafka::ERR__TIMED_OUT:
    print::debug("timed out");
    break;

  case RdKafka::ERR_NO_ERROR: {
    /* Real message */
    print::debug("Read msg at offset ", message->offset());
    std::string single_config =
        "name: \"example.co_auth\" \n"
        "domain: \"example.com\" \n"
        "challenge_type: \"auth\" \n"
        "challenge: \"auth.html\" \n"
        "password_hash: \"BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM=\" \n"
        "# sha256(\"howisbabbyformed?\")\n"
        "magic_word: old_style_back_compat\n"
        "magic_word_exceptions:\n"
        "  - \"wp-admin/admin.ajax.php\" \n"
        "  - \"wp-admin/another_script.php\" \n"
        "validity_period: 360000\n"
        "no_of_fails_to_ban: 10\n";
    challenger->load_single_dynamic_config(single_config);
  } break;
  case RdKafka::ERR__PARTITION_EOF: {
    print::debug("%% EOF reached for all  partition(s)");
  }
  break;
  case RdKafka::ERR__UNKNOWN_TOPIC: {
  }
  case RdKafka::ERR__UNKNOWN_PARTITION: {
    print::debug("Consume failed: ", message->errstr());
  } break;
  default: {
  /* Errors */
    print::debug("Consume failed: ", message->errstr());
  }
  }
}
