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
//NULL not defined in c++
#include <cstddef>
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <iomanip>

//check if the banjax.conf folder exists and is a folder indeed
#include <sys/stat.h>
//#include <libexplain/stat.h>

using namespace std;

#include <re2/re2.h>
#include <zmq.hpp>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */

#include "util.h"
#include "banjax_continuation.h"

#include "regex_manager.h"
#include "challenge_manager.h"
#include "white_lister.h"
#include "bot_sniffer.h"
#include "denialator.h"

#include "banjax.h"
#include "swabber_interface.h"
#include "ats_event_handler.h"

#define TSError_does_not_work_in_TSPluginInit

#ifdef TSError_does_not_work_in_TSPluginInit
#define TSError TSErrorAlternative
#endif

extern TSCont Banjax::global_contp;

extern const string Banjax::CONFIG_FILENAME = "banjax.conf";

extern Banjax* ATSEventHandler::banjax;

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
      filters.push_back(cur_filter);
    }
  }

  // Ask each filter what part of http transaction they are interested in
  for(const auto& cur_filter : filters) {
    all_filters_requested_part |= cur_filter->requested_info();
    all_filters_response_part  |= cur_filter->response_info();
  }
}

/**
   reload config and remake filters when traffic_line -x is executed
   the ip databes will stay untouched so banning states should
   be stay steady
*/
void Banjax::reload_config() {
    //all we need is
    // - to empty the queuse
    // - delete the filters
    // - re-read the config
    // - call filter factory

  //we need to lock this other wise somebody is deleting filter and somebody making them
  TSMutexLock(config_mutex);
  TSDebug(BANJAX_PLUGIN_NAME, "locked config lock");
    //empty all queues
  for(unsigned int i = BanjaxFilter::HTTP_START; i < BanjaxFilter::TOTAL_NO_OF_QUEUES; i++)
    task_queues[i].clear();

  //delete all filters
  for (auto filter : filters) {
      delete filter;
  }

  filters.clear();

  //reset the ip_database
  ip_database.drop_everything();

  //re-load config
  //reset config variables
  filter_config_map.clear();
  priority_map.clear();

  priorities = YAML::Node();

  current_sequential_priority = 0;

  all_filters_requested_part = 0;
  all_filters_response_part = 0;

  read_configuration();
  TSDebug(BANJAX_PLUGIN_NAME, "unlock config lock");
  TSMutexUnlock(config_mutex); //we will lock it in read_configuration again

}


/**
   Constructor

   @param banjax_config_dir path to the folder containing banjax.conf
*/
Banjax::Banjax(const string& banjax_config_dir)
  : all_filters_requested_part(0),
    all_filters_response_part(0),
    config_mutex(TSMutexCreate()),
    banjax_config_dir(banjax_config_dir),
    current_sequential_priority(0),
    swabber_interface(&ip_database)
{

  //Everything is static in ATSEventHandle so it is more like a namespace
  //than a class (we never instatiate from it). so the only reason
  //we have to create this object is to set the static reference to banjax into
  //ATSEventHandler, it is somehow the acknowledgementt that only one banjax
  //object can exist
  ATSEventHandler::banjax = this;

  /* create an TSTextLogObject to log blacklisted requests to */
  TSReturnCode error = TSTextLogObjectCreate(BANJAX_PLUGIN_NAME, TS_LOG_MODE_ADD_TIMESTAMP, &log);
  if (!log || error == TS_ERROR) {
    TSDebug(BANJAX_PLUGIN_NAME, "error while creating log");
  }

  TSDebug(BANJAX_PLUGIN_NAME, "in the beginning");

  global_contp = TSContCreate(ATSEventHandler::banjax_global_eventhandler, ip_database.db_mutex);

  BanjaxContinuation* cd = (BanjaxContinuation *) TSmalloc(sizeof(BanjaxContinuation));
  cd = new(cd) BanjaxContinuation(NULL); //no transaction attached to this cont
  TSContDataSet(global_contp, cd);

  cd->contp = global_contp;

  //For being able to be reload by traffic_line -x
  TSCont management_contp = TSContCreate(ATSEventHandler::banjax_management_handler, NULL);
  TSMgmtUpdateRegister(management_contp, BANJAX_PLUGIN_NAME);

  //creation of filters happen here
  TSMutexLock(config_mutex);
  read_configuration();
  TSMutexUnlock(config_mutex);

  //this probably should happen at the end due to multi-threading
  TSHttpHookAdd(TS_HTTP_TXN_START_HOOK, global_contp);

}

void
Banjax::read_configuration()
{
  // Read the file. If there is an error, report it and exit.
  static const  string sep = "/";

  //string banjax_dir = TSPluginDirGet(); //+ sep + BANJAX_PLUGIN_NAME;
  string absolute_config_file = /*TSInstallDirGet() + sep + */ banjax_config_dir + sep+ CONFIG_FILENAME;
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

/* Global pointer that keep track of banjax global object */
Banjax* p_banjax_plugin;

void
TSPluginInit(int argc, const char *argv[])
{
  TSPluginRegistrationInfo info;

  //set the config folder
  std::string banjax_config_dir = TSPluginDirGet();

  info.plugin_name = (char*) BANJAX_PLUGIN_NAME;
  info.vendor_name = (char*) "eQualit.ie";
  info.support_email = (char*) "info@deflect.ca";

  if (!check_ts_version(TSTrafficServerVersionGet())) {
    TSError("Plugin requires Traffic Server 3.0 or later");
    goto fatal_err;

  }

  if (argc > 1) {//then use the path specified by the arguemnt
    banjax_config_dir = argv[1];

    struct stat stat_buffer;
    if (stat(banjax_config_dir.c_str(), &stat_buffer) < 0) {
      std::string error_str = "given banjax config directory " + banjax_config_dir + " doen't exist or is unaccessible.";
      TSError(error_str.c_str());
      // int err = errno;
      // TSError(explain_errno_stat(err, banjax_config_dir.c_str(), &stat_buffer));

      goto fatal_err;

    }

    if (!S_ISDIR(stat_buffer.st_mode)) {
      std::string error_str = "given banjax config directory " + banjax_config_dir + " doesn't seem to be an actual directory";
      TSError(error_str.c_str());
      goto fatal_err;
    }

  }

  /* create the banjax object that control the whole procedure */
  p_banjax_plugin = (Banjax*)TSmalloc(sizeof(Banjax));
  p_banjax_plugin = new(p_banjax_plugin) Banjax(banjax_config_dir);

  //if everything went smoothly then register banjax
#if(TS_VERSION_NUMBER < 6000000)
  if (TSPluginRegister(TS_SDK_VERSION_3_0, &info) != TS_SUCCESS) {
#else
  if (TSPluginRegister(&info) != TS_SUCCESS) {
#endif
      TSError("[version] Plugin registration failed. \n");
     goto fatal_err;

  }

  return; //reaching this point means successfully registered

 fatal_err:
    TSError("Unable to register banjax due to a fatal error.");
    abort_traffic_server();

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
