/*
 * Banjax is an ATS plugin that:
 *                    enforce regex bans
 *                    store logs in a mysql db
 *                    run SVM on the log result
 *                    send a ban request to swabber in case of banning.
 *
 * Copyright (c) 2013 eQualit.ie under GNU AGPL v3.0 or later
 *
 * Vmon: June 2013 Initial version
 */

#include <ts/ts.h>
#include <string>
#include <iostream>
#include <sys/stat.h>

#include "transaction_data.h"
#include "banjax.h"
#include "defer.h"
#include "print.h"
#include "kafka.h"
#include <boost/asio/ip/host_name.hpp>

using namespace std;

const string CONFIG_FILENAME = "banjax.conf";

struct BanjaxPlugin {
  string config_dir;
  TSMutex reload_mutex;
  std::shared_ptr<Banjax> current_state;
  time_t restart_time;  // when the plugin was last (re)started
  time_t reload_time;   // when the plugin's config was last reloaded

  BanjaxPlugin(string config_dir)
    : config_dir(move(config_dir))
    , reload_mutex(TSMutexCreate())
    , current_state(make_shared<Banjax>(this->config_dir))
    , restart_time(time(NULL))
    , reload_time(time(NULL))
  {}

  void reload_config() {
    TSMutexLock(reload_mutex);
    auto on_exit = defer([&] { TSMutexUnlock(reload_mutex); });

    reload_time = time(NULL);

    auto swab_s = current_state->release_swabber_socket();
    auto snif_s = current_state->release_botsniffer_socket();
    auto kafka_consumer = current_state->release_kafka_consumer();

    std::shared_ptr<Banjax> new_banjax(new Banjax(config_dir, move(swab_s), move(snif_s), move(kafka_consumer)));

    // This happens atomically, so (in theory) we don't need to wrap in in mutex
    // in the handle_transaction_start hook.
    current_state = std::move(new_banjax);
  }

  int report_status() {
    if (!current_state) {
        return -1;
    }

    TSMutexLock(reload_mutex);
    auto on_exit = defer([&] { TSMutexUnlock(reload_mutex); });

    return current_state->report_status();
  }
  
  int remove_expired_challenges() {
    if (!current_state) {
        return -1;
    }

    TSMutexLock(reload_mutex);
    auto on_exit = defer([&] { TSMutexUnlock(reload_mutex); });

    return current_state->remove_expired_challenges();
  }
};

std::shared_ptr<BanjaxPlugin> g_banjax_plugin;
TSMutex scheduled_continuation_mutex;


/**
 *  stop ats due to banjax config problem
 */
static void abort_traffic_server()
{
  print::debug("Banjax was unable to start properly.");
  print::debug("preventing ATS to run cause quietly starting ATS without banjax is worst possible combination");
  TSReleaseAssert(false);
}

/**
   Read the config file and create filters whose name is
   mentioned in the config file. If you make a new filter
   you need to add it inside this function
*/
void
Banjax::build_filters()
{
  BanjaxFilter* cur_filter;

  for (const pair<int,string>& cur_filter_name : priority_map) {
    FilterConfig& cur_config = filter_config_map[cur_filter_name.second];

    try {
      if (cur_filter_name.second == REGEX_BANNER_FILTER_NAME) {
        regex_manager.reset(new RegexManager(cur_config, &regex_manager_ip_db, &swabber));
        cur_filter = regex_manager.get();
      } else if (cur_filter_name.second == CHALLENGER_FILTER_NAME){
        challenger.reset(new Challenger(banjax_config_dir, cur_config, &challenger_ip_db, &swabber, &global_ip_white_list, this));
        cur_filter = challenger.get();
      } else if (cur_filter_name.second == WHITE_LISTER_FILTER_NAME){
        white_lister.reset(new WhiteLister(cur_config, global_ip_white_list));
        cur_filter = white_lister.get();
      } else if (cur_filter_name.second == BOT_SNIFFER_FILTER_NAME){
        bot_sniffer.reset(new BotSniffer(cur_config, move(botsniffer_socket_reuse)));
        cur_filter = bot_sniffer.get();
      } else if (cur_filter_name.second == DENIALATOR_FILTER_NAME){
        denialator.reset(new Denialator(cur_config, &swabber_ip_db, &swabber, &global_ip_white_list));
        cur_filter = denialator.get();
      } else {
        print::debug("Don't know how to construct filter: \"", cur_filter_name.second, "\"");
        abort_traffic_server();
      }
    } catch (YAML::Exception& e) {
      print::debug("Error in intializing filter ", cur_filter_name.second);
      abort_traffic_server();
    }

    //at which que the filter need to be called
    for(unsigned int i = BanjaxFilter::HTTP_START; i < BanjaxFilter::TOTAL_NO_OF_QUEUES; i++) {
      if (cur_filter->queued_tasks[i]) {
        print::debug("Active task ", cur_filter->BANJAX_FILTER_NAME, " on queue:", i);
        task_queues[i].push_back(cur_filter->queued_tasks[i]);
      }
    }

    all_filters_requested_part |= cur_filter->requested_info();
    all_filters_response_part  |= cur_filter->response_info();
  }
}

static
int
handle_transaction_start(TSCont contp, TSEvent event, void *edata)
{
  if (event != TS_EVENT_HTTP_TXN_START) {
    print::debug("Txn unexpected event");
    return TS_EVENT_NONE;
  }

  TSHttpTxn txnp = (TSHttpTxn) edata;

  print::debug("Txn start");

  TSCont txn_contp;
  TransactionData *cd;

  txn_contp = TSContCreate((TSEventFunc) TransactionData::handle_transaction_change, TSMutexCreate());
  /* create the data that'll be associated with the continuation */
  cd = (TransactionData *) TSmalloc(sizeof(TransactionData));
  cd = new(cd) TransactionData(g_banjax_plugin->current_state, txnp);
  TSContDataSet(txn_contp, cd);

  TSHttpTxnHookAdd(txnp, TS_HTTP_READ_REQUEST_HDR_HOOK, txn_contp);
  TSHttpTxnHookAdd(txnp, TS_HTTP_SEND_REQUEST_HDR_HOOK, txn_contp);
  TSHttpTxnHookAdd(txnp, TS_HTTP_SEND_RESPONSE_HDR_HOOK, txn_contp);
  TSHttpTxnHookAdd(txnp, TS_HTTP_TXN_CLOSE_HOOK, txn_contp);

  TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);

  return TS_EVENT_NONE;
}

static
int handle_management(TSCont contp, TSEvent event, void *edata)
{
  (void) contp; (void) edata;
  print::debug("Reload configuration signal received");
  TSReleaseAssert(event == TS_EVENT_MGMT_UPDATE);
  g_banjax_plugin->reload_config();
  return 0;
}

std::unique_ptr<Socket> Banjax::release_swabber_socket() {
  return swabber.release_socket();
}

std::unique_ptr<Socket> Banjax::release_botsniffer_socket() {
  if (!bot_sniffer) return nullptr;
  return bot_sniffer->release_socket();
}

std::unique_ptr<KafkaConsumer> Banjax::release_kafka_consumer() {
    return std::move(kafka_consumer);
}

int
report_status_g(TSCont contp, TSEvent event, void *edata)
{
  if (!g_banjax_plugin) {
    return -1;
  }

  return g_banjax_plugin->report_status();
}

int
remove_expired_challenges_g(TSCont contp, TSEvent event, void *edata)
{
  if (!g_banjax_plugin) {
    return -1;
  }

  return g_banjax_plugin->remove_expired_challenges();
}

int
Banjax::report_status()
{
  if (!kafka_producer) {
      return -1;
  }

  time_t current_timestamp = time(NULL);

  json message;
  message["id"] = host_name;
  message["name"] = "status";
  message["num_of_host_challenges"] = challenger->dynamic_host_challenges_size();
  message["num_of_ip_challenges"] = challenger->dynamic_ip_challenges_size();
  message["timestamp"] = current_timestamp;
  message["restart_time"] =  g_banjax_plugin->restart_time;
  message["reload_time"] = g_banjax_plugin->reload_time;
  message["swabber_ip_db_size"] = swabber_ip_db.size();
  message["challenger_ip_db_size"] = challenger_ip_db.size();
  message["regex_manager_ip_db_size"] = regex_manager_ip_db.size();

  if (kafka_conf["ats_metrics_to_report"]) {
    auto metrics = kafka_conf["ats_metrics_to_report"].as<std::vector<std::string>>();

	TSMgmtInt stat_value_int = 0;
	TSMgmtFloat stat_value_float = 0;
	for (const std::string& stat_name : metrics) {
		if (TS_SUCCESS == TSMgmtIntGet(stat_name.c_str(), &stat_value_int)) {
			message[stat_name] = stat_value_int;
		} else if (TS_SUCCESS == TSMgmtFloatGet(stat_name.c_str(), &stat_value_float)) {
			message[stat_name] = stat_value_float;
        }
	}
  }


  print::debug("reporting status");
  return kafka_producer->send_message(message);
}

int
Banjax::report_pass_or_failure(const std::string& site, const std::string& ip, bool passed)
{
  if (!kafka_producer) {
      return -1;
  }

  json message;
  message["id"] = host_name;
  if (passed) {
    message["name"] = "ip_passed_challenge";
  } else {
    message["name"] = "ip_failed_challenge";
  }
  message["value_ip"] = ip;
  message["value_site"] = site;

  auto challenger_ip_state = challenger_ip_db.get_ip_state(ip);

  if (challenger_ip_state) {
    message["value_challenger_db"] = (uint64_t)*challenger_ip_state;
  } else {
    message["value_challenger_db"] = nullptr;
  }


  print::debug("reporting challenge failure for site: ", site, " and ip: ", ip);
  return kafka_producer->send_message(message);
}

int
Banjax::report_ip_banned(const std::string& site, const std::string& ip)
{
  if (!kafka_producer) {
      return -1;
  }

  json message;
  message["id"] = host_name;
  message["name"] = "ip_banned";
  message["value_ip"] = ip;
  message["value_site"] = site;

  auto challenger_ip_state = challenger_ip_db.get_ip_state(ip);

  if (challenger_ip_state) {
    message["value_challenger_db"] = (uint64_t)*challenger_ip_state;
  } else {
    message["value_challenger_db"] = nullptr;
  }


  print::debug("reporting ip_banned for site: ", site, " and ip: ", ip);
  return kafka_producer->send_message(message);
}

int
Banjax::report_if_ip_in_database(const std::string& ip)
{
  if (!kafka_producer) {
      return -1;
  }

  auto challenger_ip_state = challenger_ip_db.get_ip_state(ip);


  json message;
  message["name"] = "ip_in_database";
  message["value_ip"] = ip;

  if (challenger_ip_state) {
    message["value_challenger_db"] = (uint64_t)*challenger_ip_state;
  } else {
    message["value_challenger_db"] = nullptr;
  }

  return kafka_producer->send_message(message);
}

int
Banjax::remove_expired_challenges()
{
  if (!challenger) {
      return -1;
  }

  return challenger->remove_expired_challenges();
}

/**
   Constructor

   @param banjax_config_dir path to the folder containing banjax.conf
*/
Banjax::Banjax(const string& banjax_config_dir,
    std::unique_ptr<Socket> swabber_socket,
    std::unique_ptr<Socket> bot_sniffer_socket,
    std::unique_ptr<KafkaConsumer> kafka_consumer)
  : all_filters_requested_part(0),
    all_filters_response_part(0),
    banjax_config_dir(banjax_config_dir),
    swabber(&swabber_ip_db, move(swabber_socket)),
    botsniffer_socket_reuse(move(bot_sniffer_socket)),
    kafka_consumer(move(kafka_consumer))
{
  read_configuration();
}

void
Banjax::read_configuration()
{
  host_name = boost::asio::ip::host_name();

  // Read the file. If there is an error, report it and exit.
  static const  string sep = "/";

  string absolute_config_file = banjax_config_dir + sep + CONFIG_FILENAME;
  print::debug("Reading configuration from [", absolute_config_file, "]");

  try
  {
    cfg = YAML::LoadFile(absolute_config_file);
    process_config(cfg);
  }
  catch(YAML::BadFile& e) {
    print::debug("I/O error while reading config file [",absolute_config_file,"]: [",e.what(),"]. Make sure that file exists.");
    abort_traffic_server();
  }
  catch(YAML::ParserException& e)
  {
    print::debug("parsing error while reading config file [",absolute_config_file,"]: [",e.what(),"].");
    abort_traffic_server();
  }

  print::debug("Finished loading main conf");

  int min_priority = 0, max_priority = 0;

  //setting up priorities
  if (priorities.size()) {
    //first find the max priorities
    try {
      min_priority = priorities.begin()->second.as<int>();
      max_priority = priorities.begin()->second.as<int>();

    } catch( YAML::RepresentationException &e ) {
      print::debug("Bad config format ", e.what());
      abort_traffic_server();
    }

    for (const auto& p : priorities) {
      if (p.second.as<int>() > max_priority)
        max_priority = p.second.as<int>();

      if (p.second.as<int>() < min_priority)
        min_priority = p.second.as<int>();
    }
  }

  //now either replace priorities or add them up
  for(pair<const std::string, FilterConfig>& p : filter_config_map) {
    if (priorities[p.first]) {
      p.second.priority = priorities[p.first].as<int>();
    }
    else {
      p.second.priority += max_priority + 1;
    }

    if (priority_map.count(p.second.priority)) {
      print::debug("Priority ", p.second.priority, " has been doubly assigned");
      abort_traffic_server();
    }

    priority_map[p.second.priority] = p.first;
  }

  //(re)set swabber configuration if there is no swabber node
  //in the configuration we reset the configuration
  swabber.load_config(swabber_conf);

  //now we can make the filters
  build_filters();


  if (!kafka_conf["metadata.broker.list"]) {
    print::debug("Did not find Kafka config");
    kafka_consumer.reset();
    kafka_producer.reset();
  } else {
    print::debug("Found Kafka config");
    if (kafka_consumer == nullptr) {
      kafka_consumer = std::make_unique<KafkaConsumer>(kafka_conf, this);
    } else {
      kafka_consumer->reload_config(kafka_conf, this);
    }

    kafka_producer = std::make_unique<KafkaProducer>(this, kafka_conf);
  }

  if (report_status_interval_seconds != 0) {
    auto secs = report_status_interval_seconds;
    report_status_action = TSContScheduleEvery(TSContCreate(report_status_g, scheduled_continuation_mutex), secs * 1000ll, TS_THREAD_POOL_TASK);
    if (report_status_action != nullptr) {
        print::debug("successfully scheduled report_status_action");
    } else {
        print::debug("UNsuccessfully scheduled report_status_action");
    }
  } else {
    report_status_action = TSContScheduleEvery(TSContCreate(report_status_g, scheduled_continuation_mutex), 15ll * 1000ll, TS_THREAD_POOL_TASK);
  }

  if (remove_expired_challenges_interval_seconds != 0) {
    auto secs = remove_expired_challenges_interval_seconds;
    remove_expired_challenges_action = TSContScheduleEvery(TSContCreate(remove_expired_challenges_g, scheduled_continuation_mutex), secs * 1000ll, TS_THREAD_POOL_TASK);
    if (remove_expired_challenges_action != nullptr) {
        print::debug("successfully scheduled remove_expired_challenges_action");
    } else {
        print::debug("UNsuccessfully scheduled remove_expired_challenges_action");
    }
  } else {
    remove_expired_challenges_action = TSContScheduleEvery(TSContCreate(remove_expired_challenges_g, scheduled_continuation_mutex), 16ll * 1000ll, TS_THREAD_POOL_TASK);
  }

}


//Recursively read the entire config structure
//including inside the included files
void
Banjax::process_config(const YAML::Node& cfg)
{
  static const  string sep = "/";

  int current_sequential_priority = 0;

  // if a config_reload() happens and this section of the config goes away,
  // we don't want to keep the old config around, so zero it out here.
  kafka_conf = YAML::Node();

  report_status_interval_seconds = 0;
  remove_expired_challenges_interval_seconds = 0;

  auto is_filter = [](const std::string& s) {
    return std::find( all_filters_names.begin()
                    , all_filters_names.end(), s) != all_filters_names.end();
  };

  for (YAML::const_iterator it = cfg.begin(); it != cfg.end(); ++it) {
    try {
      std::string node_name = it->first.as<std::string>();

      if (is_filter(node_name)) {
        // If it's a filter see if it is already in the list
        if (filter_config_map.find(node_name) == filter_config_map.end()) {
          filter_config_map[node_name].priority = current_sequential_priority;
          current_sequential_priority++;
        }

        filter_config_map[node_name].config_node_list.push_back(it);
      }
      else if (node_name == "swabber") {
        //we simply send swabber configuration to swabber
        //if it doesn't exists it fails to default
        swabber_conf.config_node_list.push_back(it);
      }
      else if (node_name == "priority") {
        // Store it as priority config.  for now we fire error if priority is
        // double defined
        if (priorities.size()) {
          print::debug("Double definition of priorities. only one priority list is allowed.");
          abort_traffic_server();
        }

        priorities = Clone(it->second);
      }
      else if (node_name == "include") {
        for(const auto& sub : it->second) {
          string inc_loc = banjax_config_dir + sep + sub.as<std::string>();
          print::debug("Reading configuration from [", inc_loc, "]");
          try {
            process_config(YAML::LoadFile(inc_loc));
          }
          catch(YAML::BadFile& e) {
            print::debug("I/O error while reading config file [",inc_loc,"]: [",e.what(),"]. Make sure that file exists.");
            abort_traffic_server();
          }
          catch(YAML::ParserException& e)  {
            print::debug("Parsing error while reading config file [",inc_loc,"]: [",e.what(),"].");
            abort_traffic_server();
          }
        }
      } else if (node_name == "kafka") {
        kafka_conf = it->second;
      } else if (node_name == "report_status_interval_seconds") {
        report_status_interval_seconds = it->second.as<int>();
      } else if (node_name == "remove_expired_challenges_interval_seconds") {
        remove_expired_challenges_interval_seconds = it->second.as<int>();
      }
      else { //unknown node
        print::debug("Unknown config node ", node_name);
        abort_traffic_server();
      }
    }
    catch( YAML::RepresentationException &e ) {
      print::debug("Bad config format ", e.what());
      abort_traffic_server();
    }
  }
}

void
Banjax::kafka_message_consume(const json& message) {
  auto command_name_it = message.find("name");
  if (command_name_it == message.end()) {
      print::debug("kafka command has no 'name' key");
      return;
  }
  auto value_it = message.find("value");
  if (value_it == message.end()) {
    print::debug("kafka command has no 'value' key");
    return;
  }
  if (!challenger) {
    print::debug("null challenger at time of kafka_message_consume()");
    return;
  }

  if (*command_name_it == "challenge_host") {
    std::string website = *value_it;
    challenger->load_single_host_challenge(website);
  } else if (*command_name_it == "challenge_ip") {
    std::string ip = *value_it;
    report_if_ip_in_database(ip);
    challenger->load_single_ip_challenge(ip);
  } else {
    print::debug("kafka command not 'challenge_host' or 'challenge_ip'");
  }
}

static void destroy_g_banjax_plugin() {
  g_banjax_plugin.reset();
}

Banjax::~Banjax() {
  if (report_status_action != nullptr) {
    print::debug("Cancelling old report_status_action");
    TSActionCancel(report_status_action);
  }
  print::debug("After Cancelling old report_status_action");

  if (remove_expired_challenges_action != nullptr) {
    print::debug("Cancelling old remove_expired_challenges_action");
    TSActionCancel(remove_expired_challenges_action);
  }
  print::debug("After Cancelling old remove_expired_challenges_action");

}

void
TSPluginInit(int argc, const char *argv[])
{
  scheduled_continuation_mutex = TSMutexCreate();
  TSPluginRegistrationInfo info;

  info.plugin_name = (char*) BANJAX_PLUGIN_NAME;
  info.vendor_name = (char*) "eQualit.ie";
  info.support_email = (char*) "info@deflect.ca";

  if (!check_ts_version(TSTrafficServerVersionGet())) {
    print::debug("Plugin requires Traffic Server 3.0 or later");
    return abort_traffic_server();
  }

  {
    std::stringstream ss;
    for (int i = 0; i < argc; i++) {
      if (i != 0) ss << ", ";
      ss << "\"" << argv[i] << "\"";
    }
    print::debug("TSPluginInit args: ", ss.str());
  }

  // Set the config folder to a default value (can be modified by argv[1])
  std::string banjax_config_dir = TSPluginDirGet();

  if (argc > 1) {
    banjax_config_dir = argv[1];

    struct stat stat_buffer;
    if (stat(banjax_config_dir.c_str(), &stat_buffer) < 0) {
      std::string error_str = "given banjax config directory " + banjax_config_dir + " doen't exist or is unaccessible.";
      print::debug(error_str.c_str());
      return abort_traffic_server();
    }

    if (!S_ISDIR(stat_buffer.st_mode)) {
      std::string error_str = "given banjax config directory " + banjax_config_dir + " doesn't seem to be an actual directory";
      print::debug(error_str.c_str());
      return abort_traffic_server();
    }
  }

  //if everything went smoothly then register banjax
#if(TS_VERSION_NUMBER < 6000000)
  if (TSPluginRegister(TS_SDK_VERSION_3_0, &info) != TS_SUCCESS) {
#else
  if (TSPluginRegister(&info) != TS_SUCCESS) {
#endif
    print::debug("[version] Plugin registration failed.");
    return abort_traffic_server();
  }

  /* create the banjax object that control the whole procedure */
  g_banjax_plugin.reset(new BanjaxPlugin{banjax_config_dir});
  atexit(destroy_g_banjax_plugin);

  // Start handling transactions
  TSCont contp = TSContCreate(handle_transaction_start, nullptr);
  TSHttpHookAdd(TS_HTTP_TXN_START_HOOK, contp);

  // Handle reload by traffic_line -x
  TSCont management_contp = TSContCreate(handle_management, nullptr);
  TSMgmtUpdateRegister(management_contp, BANJAX_PLUGIN_NAME);

}
