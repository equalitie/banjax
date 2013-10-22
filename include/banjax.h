/*
 * These are definitions that are used in the main module (or shared by more 
 * modules)
 * Vmon: June 2013
 */

#ifndef BANJAX_H
#define BANJAX_H
#include <libconfig.h++>
#include <string>
#include <list>

class ATSEventHandler;
class BanjaxFilter;

class Banjax
{
  friend class ATSEventHandler;
  
 protected:
  //requests
  TSTextLogObject log;
  static TSCont global_contp;
  
  IPDatabase ip_database;
  
  std::list<BanjaxFilter*> filters;


  //configuration
  static const std::string CONFIG_FILENAME;
  //libconfig object
  libconfig::Config cfg;

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
  void filter_factory(const std::string& banjax_dir, const libconfig::Setting& main_root);

  uint64_t all_filters_requested_part;

 public:
  uint64_t which_parts_are_requested() { return all_filters_requested_part;}
  //The name of pluging to be used for TSDebug and folders, etc.
  static const std::string BANJAX_PLUGIN_NAME;
  /* Constructor */
  Banjax();

};

#endif /*banjax.h*/
