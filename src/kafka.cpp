#include "kafka.h"
#include <iostream>
#include <librdkafka/rdkafkacpp.h>
#include <boost/asio/ip/host_name.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

KafkaProducer::KafkaProducer() {
  print::debug("KafkaProducer default constructor");
}

void KafkaProducer::load_config(YAML::Node &new_config) {
  if (new_config.Type() != YAML::NodeType::Map) {
      print::debug("KafkaProducer::load_config requires a YAML::Map");
    throw;
  }

  auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

  auto brokers = new_config["brokers"].as<std::string>();
  std::string errstr;
  if (conf->set("metadata.broker.list", brokers, errstr) !=
      RdKafka::Conf::CONF_OK) {
      print::debug("KafkaProducer: bad 'brokers' config: ", errstr);
    throw;
  }

  failed_challenge_topic = new_config["failed_challenge_topic"].as<std::string>();
  status_topic =           new_config["status_topic"].as<std::string>();

  rdk_producer_for_failed_challenges.reset(RdKafka::Producer::create(conf, errstr));
  if (!rdk_producer_for_failed_challenges) {
      print::debug("KafkaProducer: failed to create Producer (for failed challenges)");
    throw;
  }

  // XXX ugly duplication
  rdk_producer_for_statuses.reset(RdKafka::Producer::create(conf, errstr));
  if (!rdk_producer_for_statuses) {
      print::debug("KafkaProducer: failed to create Producer (for statuses)");
    throw;
  }

  host_name = boost::asio::ip::host_name();

  print::debug("KafkaProducer load_config done");
}

void KafkaProducer::report_failure(const std::string& site, const std::string& ip) {
  json message;
  message["id"] = host_name;
  message["name"] = "ip_failed_challenge";
  message["value"] = ip;
  std::string serialized_message = message.dump();
  size_t serialized_message_size = message.dump().size();

  RdKafka::ErrorCode err = rdk_producer_for_failed_challenges->produce(
      /* Topic name */
      failed_challenge_topic,
      /* Any Partition: the builtin partitioner will be
              * used to assign the message to a topic based
              * on the message key, or random partition if
              * the key is not set. */
      (int)RdKafka::Topic::PARTITION_UA,
      /* Make a copy of the value */
      (int)RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
      /* Value */
      (void*)serialized_message.c_str(), serialized_message_size,
      /* Key */
      nullptr, 0,
      /* Timestamp (defaults to current time) */
      0,
      /* Message headers, if any */
      NULL);
  if (err == RdKafka::ERR_NO_ERROR) {
      print::debug("reported challenge failure for site: ", site, " and ip: ", ip, " to topic: ", failed_challenge_topic);
  } else {
      print::debug(": Failed to produce()");
  }
}

void KafkaProducer::report_status() {
  json message;
  message["id"] = host_name;

  std::string serialized_message = message.dump();
  size_t serialized_message_size = message.dump().size();

  RdKafka::ErrorCode err = rdk_producer_for_statuses->produce(
      /* Topic name */
      status_topic,
      /* Any Partition: the builtin partitioner will be
              * used to assign the message to a topic based
              * on the message key, or random partition if
              * the key is not set. */
      (int)RdKafka::Topic::PARTITION_UA,
      /* Make a copy of the value */
      (int)RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
      /* Value */
      (void*)serialized_message.c_str(), serialized_message_size,
      /* Key */
      nullptr, 0,
      /* Timestamp (defaults to current time) */
      0,
      /* Message headers, if any */
      NULL);
  if (err == RdKafka::ERR_NO_ERROR) {
      print::debug("reported status");
  } else {
      print::debug(": Failed to report status()");
  }
}

void
KafkaConsumer::reload_config(YAML::Node& config, Banjax* banjax) {
    TSMutexLock(stored_config_lock);
    auto on_scope_exit = defer([&] { TSMutexUnlock(stored_config_lock); });
    stored_config = config;
    banjax = banjax;
    config_valid = false;
}


KafkaConsumer::KafkaConsumer(YAML::Node &new_config, Banjax* banjax)
  : stored_config_lock(TSMutexCreate()),
    stored_config(new_config),
    banjax(banjax)
{
    thread_handle = std::thread([=] {
        print::debug("hello from lambda");
        while (!shutting_down) {
            print::debug("(RE)LOADING CONFIG");
            auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
            auto tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
            std::vector<std::string> topics;  // annoyingly a vector when we really just need one
            std::string errstr;
            {
                TSMutexLock(stored_config_lock);
                auto on_scope_exit = defer([&] { TSMutexUnlock(stored_config_lock); });
                if (stored_config.Type() != YAML::NodeType::Map) {
                print::debug("KafkaConsumer::load_config requires a YAML::Map");
                    throw;
                }

                auto topic = stored_config["challenge_host_topic"].as<std::string>();
                topics.push_back(topic);
        
                auto brokers = stored_config["brokers"].as<std::string>();
                if (conf->set("metadata.broker.list", brokers, errstr) !=
                    RdKafka::Conf::CONF_OK) {
                    print::debug("KafkaConsumer: bad 'brokers' config: ", errstr);
                    throw;
                }

                host_name = boost::asio::ip::host_name();
                // we want every banjax instance to see every message. this means every banjax instance
                // needs its own group id. so i'm using the hostname.
                if (conf->set("group.id", host_name, errstr) != RdKafka::Conf::CONF_OK) {
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
          
            RdKafka::ErrorCode resp = consumer->subscribe(topics);
            if (resp != RdKafka::ERR_NO_ERROR) {
                print::debug("Failed to start consumer: ", RdKafka::err2str(resp));
                throw; // XXX inside this thread?...
            }

            uint16_t times_around_loop = 0;
            while (config_valid && !shutting_down) {
                std::cerr << "BLOCKING" << std::endl;
                RdKafka::Message *msg = consumer->consume(2000);
                msg_consume(msg, NULL);
                delete msg;
                banjax->get_challenger()->remove_expired_challenges();
                // XXX this is not how i would have designed some scheduled task from the start, but
                // i don't feel like spending forever redesigning this properly atm.
                if (++times_around_loop > 15) {
                    banjax->get_producer()->report_status();
                    times_around_loop = 0;
                }
            }

            print::debug("BEFORE CLOSE");
            consumer->close();
            print::debug("AFTER CLOSE");
        }
        print::debug("THREAD EXITING");
    });
    print::debug("hello from OUTSIDE lambda");
}


void KafkaConsumer::msg_consume(RdKafka::Message *message, void *opaque) {
    print::debug("MSG_CONSUME()");
  switch (message->err()) {
  case RdKafka::ERR__TIMED_OUT:
    print::debug("timed out");
    break;

  case RdKafka::ERR_NO_ERROR: {
    /* Real message */
    print::debug("Read msg at offset ", message->offset());
    print::debug("msg json: ", (char*)message->payload());
    json message_dict;
    try {
      message_dict = json::parse((char*)message->payload());
    } catch (json::exception& e) {
      print::debug("kafka message not json: ", (char*)message->payload());
      return;
    }

    auto command_name_it = message_dict.find("name");
    if (command_name_it == message_dict.end() || (*command_name_it != "challenge_host")) {
      print::debug("kafka command not of 'challenge_host' type");
      return;
    }

    auto value_it = message_dict.find("value");
    if (value_it == message_dict.end()) {
      print::debug("kafka command has no 'value'");
      return;
    }

    std::string website = *value_it;
    banjax->get_challenger()->load_single_dynamic_config(website, stored_config["dynamic_challenger_config"]);
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
