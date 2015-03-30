/*
 * These are definitions that are used in the main module (or shared by more 
 * modules)
 * Vmon: June 2013
 */

#ifndef BANJAX_H
#define BANJAX_H
#include <libconfig.h++>
#include <yaml-cpp/yaml.h>
#include <string>
#include <list>

class ATSEventHandler;
class BanjaxFilter;

#include "ip_database.h"
#include "swabber_interface.h"
#include "transaction_muncher.h"
#include "banjax_filter.h"

//Everything is static in ATSEventHandler so the only reason
//we have to create this object is to set the static reference to banjax into 
//ATSEventHandler, it is somehow the acknowledgementt that only one banjax 
//object can exist
class Banjax
{
  friend class ATSEventHandler;

 public:
  typedef std::list<FilterTask> TaskQueue;
  
 protected:
  //requests
  TSTextLogObject log;
  static TSCont global_contp;
  
  IPDatabase ip_database;
  SwabberInterface swabber_interface;
  
  std::list<BanjaxFilter*> filters;
  TaskQueue task_queues[BanjaxFilter::TOTAL_NO_OF_QUEUES];

  //configuration
  static const std::string CONFIG_FILENAME;
  //libconfig object
  YAML::Node cfg;

  /* open the mysql database and read the configs from the database
     this include the regex and l2b models
  */
  void read_configuration();
  
  /**
     Read the config file and create filters whose name is
     mentioned in the config file. If you make a new filter
     you need to add it inside this function
     
     @param banjx_dir the directory that contains banjax related files to be
                      passed to each filter
     @param main_root is libconfig++ ref to the root of
                      config file
  */
  void filter_factory(const std::string& banjax_dir, YAML::Node cfg);

  uint64_t all_filters_requested_part;
  uint64_t all_filters_response_part;

 public:
  uint64_t which_parts_are_requested() { return all_filters_requested_part;}
  uint64_t which_response_parts_are_requested() { return all_filters_response_part;}
  /* Constructor */
  Banjax();

};

#endif /*banjax.h*/
