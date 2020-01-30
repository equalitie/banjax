#pragma once

#include "banjax_filter.h"
#include <librdkafka/rdkafkacpp.h>
#include <thread>
#include "banjax.h"

class KafkaProducer {
public:
  KafkaProducer();
  KafkaProducer(std::unique_ptr<RdKafka::Producer> old_rdk_producer,
                FilterConfig &old_filter_config);
  void load_config(YAML::Node &new_config);

private:
  std::unique_ptr<RdKafka::Producer> old_rdk_producer;
  std::unique_ptr<RdKafka::Producer> current_rdk_producer;

  YAML::Node old_config;
};


class KafkaConsumer {
public:
  KafkaConsumer(YAML::Node &new_config, std::shared_ptr<Challenger> challenger);
  void reload_config(YAML::Node &config, std::shared_ptr<Challenger> challenger);
  void shutdown() { shutting_down = true; thread_handle.join(); };
  ~KafkaConsumer() { shutdown(); };

private:
  bool config_valid;
  bool shutting_down = false;

  TSMutex stored_config_lock;
  YAML::Node stored_config;
  std::shared_ptr<Challenger> stored_challenger;
  std::thread thread_handle;
};

void msg_consume(RdKafka::Message *message, void *opaque, std::shared_ptr<Challenger> challenger);

