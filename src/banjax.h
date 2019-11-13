/*
 * These are definitions that are used in the main module (or shared by more
 * modules)
 * Vmon: June 2013
 */

#ifndef BANJAX_H
#define BANJAX_H
#include <yaml-cpp/yaml.h>
#include <string>
#include <list>

class BanjaxFilter;

#include "ip_db.h"
#include "swabber_interface.h"
#include "transaction_muncher.h"
#include "banjax_filter.h"
#include "global_white_list.h"
#include "regex_manager.h"
#include "challenge_manager.h"

class Banjax
{
public:
  //it keeps all part of requests and responses which is
  //requested by filter at initialization for later
  //fast use
  uint64_t all_filters_requested_part;

protected:
  uint64_t all_filters_response_part;

  std::string banjax_config_dir; //this keeps the folder contains banjax.conf and banjax.d folder

  YAML::Node cfg;
  YAML::Node priorities;

  // Store all configs related to a filter in different yaml nodes (in
  // different files maybe).
  std::map<std::string, FilterConfig> filter_config_map;

  // Keep swabber configuration.
  FilterConfig swabber_conf;

  // Ordering and accessing filters by priority.
  std::map<int, std::string> priority_map;

  // List of privileged IPs.
  GlobalWhiteList global_ip_white_list;

  //requests
  TSTextLogObject log;

  SwabberInterface::IpDb swabber_ip_db;
  ChallengeManager::IpDb challenger_ip_db;
  RegexManager::IpDb     regex_manager_ip_db;

  SwabberInterface swabber_interface;

  // This keeps the list of all created filter objects so we can delete them on
  // re-load.
  std::list<std::unique_ptr<BanjaxFilter>> filters;
  /**
     open the mysql database and read the configs from the database
     this include the regex and l2b models
  */
  void read_configuration();

  //Recursively read the entire config structure
  //including inside the included files
  void process_config(const YAML::Node& cfg);

  /**
     Read the config file and create filters whose name is
     mentioned in the config file. If you make a new filter
     you need to add it inside this function

     @param banjx_dir the directory that contains banjax related files to be
                      passed to each filter
     @param main_root is libconfig++ ref to the root of
                      config file
  */
  void filter_factory();

  /**
     reload config and remake filters when traffic_line -x is executed
     the ip databes will stay untouched so banning states should
     be stay steady
  */
  void reload_config();

public:
  typedef std::list<BanjaxFilter*> TaskQueue;

  TaskQueue task_queues[BanjaxFilter::TOTAL_NO_OF_QUEUES];

  uint64_t which_parts_are_requested() { return all_filters_requested_part;}
  uint64_t which_response_parts_are_requested() { return all_filters_response_part;}
  /**
     Constructor

     @param banjax_config_dir path to the folder containing banjax.conf
   */
  Banjax(const std::string& banjax_config_dir, std::unique_ptr<SwabberInterface::Socket> s = nullptr);

  std::unique_ptr<SwabberInterface::Socket> release_swabber_socket();
};

#endif /*banjax.h*/
