#pragma once

#include "banjax_filter.h"
#include <librdkafka/rdkafkacpp.h>
#include <thread>
#include "banjax.h"
#include "banjax_interface.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;


class KafkaProducer {
public:
  KafkaProducer(BanjaxInterface* banjax, YAML::Node& config);
  int send_message(const json& message);

private:
  std::unique_ptr<RdKafka::Producer> rdk_producer;
  std::string report_topic;
  BanjaxInterface* banjax;
};


class KafkaConsumer {
public:
  KafkaConsumer(YAML::Node &new_config, BanjaxInterface* banjax);
  void reload_config(YAML::Node &config, BanjaxInterface* banjax);
  void msg_consume(std::unique_ptr<RdKafka::Message> message, void *opaque);
  void shutdown();
  ~KafkaConsumer() { shutdown(); };

private:
  bool config_valid;
  bool shutting_down = false;

  TSMutex stored_config_lock;
  YAML::Node stored_config;
  BanjaxInterface* banjax;
  std::thread thread_handle;
};


// release_kafka_consumer()
// Banjax(..., kafka_consumer)
// read_configuration()
// kafka_consumer->reload_config()
