#pragma once

#include "banjax_filter.h"
#include <librdkafka/rdkafkacpp.h>
#include <thread>
#include "banjax.h"


// XXX should maybe be called KafkaProducers (plural)?
class KafkaProducer {
public:
  KafkaProducer();
  void load_config(YAML::Node& new_config);
  void report_failure(const std::string& site, const std::string& ip);
  void report_status();

private:
  std::unique_ptr<RdKafka::Producer> rdk_producer_for_failed_challenges;
  std::string report_topic;
  std::string host_name;
};


class KafkaConsumer {
public:
  KafkaConsumer(YAML::Node &new_config, Banjax* banjax);
  void reload_config(YAML::Node &config, Banjax* banjax);
  void msg_consume(RdKafka::Message *message, void *opaque);
  void shutdown() { shutting_down = true; thread_handle.join(); };
  ~KafkaConsumer() { shutdown(); };

private:
  bool config_valid;
  bool shutting_down = false;

  TSMutex stored_config_lock;
  YAML::Node stored_config;
  // std::shared_ptr<Challenger> stored_challenger;
  Banjax* banjax; // XXX i'd rather it be a reference, but i can't reassign a reference...
  std::thread thread_handle;
  std::string host_name;
};


