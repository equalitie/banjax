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
  KafkaProducer(Banjax* banjax, YAML::Node& config);
  int send_message(const json& message);

private:
  std::unique_ptr<RdKafka::Producer> rdk_producer;
  std::string report_topic;
  Banjax* banjax; // XXX i'd rather it be a reference, but i can't reassign a reference...
};


class KafkaConsumer {
public:
  KafkaConsumer(YAML::Node &new_config, BanjaxInterface* banjax);
  void reload_config(YAML::Node &config, BanjaxInterface* banjax);
  void msg_consume(std::unique_ptr<RdKafka::Message> message, void *opaque);
  void shutdown() { shutting_down = true; thread_handle.join(); };
  ~KafkaConsumer() { shutdown(); };

private:
  bool config_valid;
  bool shutting_down = false;

  TSMutex stored_config_lock;
  YAML::Node stored_config;
  BanjaxInterface* banjax; // XXX i'd rather it be a reference, but i can't reassign a reference...
  std::thread thread_handle;
};


