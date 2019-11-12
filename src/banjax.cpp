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
#include <vector>
#include <list>
#include <iostream>
#include <iomanip>

//check if the banjax.conf folder exists and is a folder indeed
#include <sys/stat.h>

using namespace std;

#include <re2/re2.h>
#include <zmq.hpp>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */

#include "util.h"
#include "transaction_data.h"

#include "regex_manager.h"
#include "challenge_manager.h"
#include "white_lister.h"
#include "bot_sniffer.h"
#include "denialator.h"

#include "banjax.h"
#include "swabber_interface.h"

#define TSError_does_not_work_in_TSPluginInit

#ifdef TSError_does_not_work_in_TSPluginInit
#define TSError TSErrorAlternative
#endif

const string CONFIG_FILENAME = "banjax.conf";

struct BanjaxPlugin {
  string config_dir;
  TSMutex reload_mutex;
  std::shared_ptr<Banjax> current_state;

  BanjaxPlugin(string config_dir)
    : config_dir(move(config_dir))
    , reload_mutex(TSMutexCreate())
    , current_state(make_shared<Banjax>(this->config_dir))
  {}
};

std::shared_ptr<BanjaxPlugin> g_banjax_plugin;

void TSErrorAlternative(const char* fmt, ...)
{
  va_list arglist;
  va_start(arglist, fmt);
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, fmt, arglist);
  fprintf(stderr, "\n");
  va_end(arglist);
}

/**
   Read the config file and create filters whose name is
   mentioned in the config file. If you make a new filter
   you need to add it inside this function
*/
void
Banjax::filter_factory()
{
  BanjaxFilter* cur_filter;

  for (const pair<int,string>& cur_filter_name : priority_map) {
    FilterConfig& cur_config = filter_config_map[cur_filter_name.second];

    try {
      if (cur_filter_name.second == REGEX_BANNER_FILTER_NAME) {
        cur_filter = new RegexManager(banjax_config_dir, cur_config, &ip_database, &swabber_interface);
      } else if (cur_filter_name.second == CHALLENGER_FILTER_NAME){
        cur_filter = new ChallengeManager(banjax_config_dir, cur_config, &ip_database, &swabber_interface, &global_ip_white_list);
      } else if (cur_filter_name.second == WHITE_LISTER_FILTER_NAME){
        cur_filter = new WhiteLister(banjax_config_dir, cur_config, global_ip_white_list);
      } else if (cur_filter_name.second == BOT_SNIFFER_FILTER_NAME){
        cur_filter = new BotSniffer(banjax_config_dir, cur_config);
      } else if (cur_filter_name.second == DENIALATOR_FILTER_NAME){
        cur_filter = new Denialator(banjax_config_dir, cur_config, &ip_database, &swabber_interface, &global_ip_white_list);
      } else {
        TSError(("don't know how to construct requested filter " + cur_filter_name.second).c_str());
        abort_traffic_server();
      }
    } catch (YAML::Exception& e) {
      TSError(("error in intializing filter " + cur_filter_name.second).c_str());
      abort_traffic_server();
    }

    //at which que the filter need to be called
    for(unsigned int i = BanjaxFilter::HTTP_START; i < BanjaxFilter::TOTAL_NO_OF_QUEUES; i++) {
      if (cur_filter->queued_tasks[i]) {
        TSDebug(BANJAX_PLUGIN_NAME, "active task %s %u", cur_filter->BANJAX_FILTER_NAME.c_str(), i);
        task_queues[i].push_back(cur_filter->queued_tasks[i]);
      }
    }

    if(cur_filter){
      filters.emplace_back(cur_filter);
    }
  }

  // Ask each filter what part of http transaction they are interested in
  for(const auto& cur_filter : filters) {
    all_filters_requested_part |= cur_filter->requested_info();
    all_filters_response_part  |= cur_filter->response_info();
  }
}

static
int
handle_transaction_start(TSCont contp, TSEvent event, void *edata)
{
  if (event != TS_EVENT_HTTP_TXN_START) {
    TSDebug(BANJAX_PLUGIN_NAME, "txn unexpected event" );
    return TS_EVENT_NONE;
  }

  TSHttpTxn txnp = (TSHttpTxn) edata;

  TSDebug(BANJAX_PLUGIN_NAME, "txn start");

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
  TSDebug(BANJAX_PLUGIN_NAME, "reload configuration signal received");
  TSReleaseAssert(event == TS_EVENT_MGMT_UPDATE);

  TSMutexLock(g_banjax_plugin->reload_mutex);

  auto s = g_banjax_plugin->current_state->release_swabber_socket();
  std::shared_ptr<Banjax> new_banjax(new Banjax(g_banjax_plugin->config_dir, move(s)));

  // This happens atomically, so (in theory) we don't need to wrap in in mutex
  // in the handle_transaction_start hook.
  g_banjax_plugin->current_state = std::move(new_banjax);

  TSMutexUnlock(g_banjax_plugin->reload_mutex);

  return 0;
}

std::unique_ptr<SwabberInterface::Socket> Banjax::release_swabber_socket() {
  return swabber_interface.release_socket();
}

/**
   Constructor

   @param banjax_config_dir path to the folder containing banjax.conf
*/
Banjax::Banjax(const string& banjax_config_dir,
    std::unique_ptr<SwabberInterface::Socket> swabber_socket)
  : all_filters_requested_part(0),
    all_filters_response_part(0),
    banjax_config_dir(banjax_config_dir),
    swabber_interface(&ip_database, move(swabber_socket))
{
  /* create an TSTextLogObject to log blacklisted requests to */
  TSReturnCode error = TSTextLogObjectCreate(BANJAX_PLUGIN_NAME, TS_LOG_MODE_ADD_TIMESTAMP, &log);
  if (!log || error == TS_ERROR) {
    TSDebug(BANJAX_PLUGIN_NAME, "error while creating log");
  }

  TSDebug(BANJAX_PLUGIN_NAME, "reading configuration");

  // Creation of filters happen here
  read_configuration();
}

void
Banjax::read_configuration()
{
  // Read the file. If there is an error, report it and exit.
  static const  string sep = "/";

  string absolute_config_file = banjax_config_dir + sep + CONFIG_FILENAME;
  TSDebug(BANJAX_PLUGIN_NAME, "Reading configuration from [%s]", absolute_config_file.c_str());

  try
  {
    cfg = YAML::LoadFile(absolute_config_file);
    process_config(cfg);
  }
  catch(YAML::BadFile& e) {
    TSError("I/O error while reading config file [%s]: [%s]. Make sure that file exists.", absolute_config_file.c_str(), e.what());
    abort_traffic_server();
  }
  catch(YAML::ParserException& e)
  {
    TSError("parsing error while reading config file [%s]: [%s].", absolute_config_file.c_str(), e.what());
    abort_traffic_server();
  }

  TSDebug(BANJAX_PLUGIN_NAME, "Finished loading main conf");

  int min_priority = 0, max_priority = 0;

  //setting up priorities
  if (priorities.size()) {
    //first find the max priorities
    try {
      min_priority = priorities.begin()->second.as<int>();
      max_priority = priorities.begin()->second.as<int>();

    } catch( YAML::RepresentationException &e ) {
      TSError(("bad config format " +  (string)e.what()).c_str());
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
      TSError(("Priority " + to_string(p.second.priority) + " has been doubly assigned").c_str());
      abort_traffic_server();
    }

    priority_map[p.second.priority] = p.first;
  }

  //(re)set swabber configuration if there is no swabber node
  //in the configuration we reset the configuration
  swabber_interface.load_config(swabber_conf);
  //now we can make the filters
  filter_factory();
}


//Recursively read the entire config structure
//including inside the included files
void
Banjax::process_config(const YAML::Node& cfg)
{
  static const  string sep = "/";

  int current_sequential_priority = 0;

  for (YAML::const_iterator it = cfg.begin(); it != cfg.end(); ++it) {
    try {
      std::string node_name = it->first.as<std::string>();
      if (std::find(all_filters_names.begin(), all_filters_names.end(), node_name)!=all_filters_names.end()) {

        // If it's a filter see if it is already in the list
        if (filter_config_map.find(node_name) == filter_config_map.end()) {
          filter_config_map[node_name].priority = current_sequential_priority;
          current_sequential_priority++;
        }

        filter_config_map[node_name].config_node_list.push_back(it);

      } else if (node_name == "swabber") {
        //we simply send swabber configuration to swabber
        //if it doesn't exists it fails to default
        swabber_conf.config_node_list.push_back(it);

      }
      else if (node_name == "priority") {
        //store it as priority config.
        //for now we fire error if priority is double
        //defined
        if (priorities.size()) {
          TSError("double definition of priorities. only one priority list is allowed.");

          abort_traffic_server();
        }

        priorities = Clone(it->second);

      } else if (node_name == "include") {
        for(const auto& sub : it->second) {
          string inc_loc = banjax_config_dir + sep + sub.as<std::string>();
          TSDebug(BANJAX_PLUGIN_NAME, "Reading configuration from [%s]", inc_loc.c_str());
          try {
            YAML::Node sub_cfg = YAML::LoadFile(inc_loc);
            process_config(sub_cfg);
          }
          catch(YAML::BadFile& e) {
            TSError("I/O error while reading config file [%s]: [%s]. Make sure that file exists.", inc_loc.c_str(), e.what());
            abort_traffic_server();
          }
          catch(YAML::ParserException& e)  {
            TSError("Parsing error while reading config file [%s]: [%s].", inc_loc.c_str(), e.what());
            abort_traffic_server();
          }
        }
      } else { //unknown node
        TSError(("unknown config node " + node_name).c_str());
        abort_traffic_server();
      }
    } catch( YAML::RepresentationException &e ) {
      TSError(("bad config format " +  (string)e.what()).c_str());
      abort_traffic_server();
    }
  } //for all nodes
};

static void destroy_g_banjax_plugin() {
  g_banjax_plugin.reset();
}

void
TSPluginInit(int argc, const char *argv[])
{
  TSPluginRegistrationInfo info;

  info.plugin_name = (char*) BANJAX_PLUGIN_NAME;
  info.vendor_name = (char*) "eQualit.ie";
  info.support_email = (char*) "info@deflect.ca";

  if (!check_ts_version(TSTrafficServerVersionGet())) {
    TSError("Plugin requires Traffic Server 3.0 or later");
    return abort_traffic_server();
  }

  {
    std::stringstream ss;
    for (int i = 0; i < argc; i++) {
      ss << "\"" << argv[i] << "\"";
      if (i != argc + 1) ss << ", ";
    }
    TSDebug(BANJAX_PLUGIN_NAME, "TSPluginInit args: %s", ss.str().c_str());
  }

  // Set the config folder to a default value (can be modified by argv[1])
  std::string banjax_config_dir = TSPluginDirGet();

  if (argc > 1) {
    banjax_config_dir = argv[1];

    struct stat stat_buffer;
    if (stat(banjax_config_dir.c_str(), &stat_buffer) < 0) {
      std::string error_str = "given banjax config directory " + banjax_config_dir + " doen't exist or is unaccessible.";
      TSError(error_str.c_str());
      return abort_traffic_server();
    }

    if (!S_ISDIR(stat_buffer.st_mode)) {
      std::string error_str = "given banjax config directory " + banjax_config_dir + " doesn't seem to be an actual directory";
      TSError(error_str.c_str());
      return abort_traffic_server();
    }
  }

  //if everything went smoothly then register banjax
#if(TS_VERSION_NUMBER < 6000000)
  if (TSPluginRegister(TS_SDK_VERSION_3_0, &info) != TS_SUCCESS) {
#else
  if (TSPluginRegister(&info) != TS_SUCCESS) {
#endif
    TSError("[version] Plugin registration failed. \n");
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

/**
 *  stop ats due to banjax config problem
 */
void abort_traffic_server()
{
  TSError("Banjax was unable to start properly");
  TSError("preventing ATS to run cause quietly starting ats without banjax is worst possible combination");
  TSReleaseAssert(false);
}
