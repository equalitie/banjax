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
using namespace std;

#include <re2/re2.h>
#include <zmq.hpp>

#include "util.h"
#include "banjax_continuation.h"

#include "regex_manager.h"
#include "challenge_manager.h"
#include "white_lister.h"

#include "banjax.h"
#include "swabber_interface.h"
#include "ats_event_handler.h"

extern TSCont Banjax::global_contp;

extern const string Banjax::CONFIG_FILENAME = "banjax.conf";
extern const string Banjax::BANJAX_PLUGIN_NAME = "banjax";

/**
   Read the config file and create filters whose name is
   mentioned in the config file. If you make a new filter
   you need to add it inside this function
*/
void
Banjax::filter_factory(const string& banjax_dir, const libconfig::Setting& main_root)
{
  int filter_count = main_root.getLength();
  
  for(int i = 0; i < filter_count; i++) {
    string cur_filter_name = main_root[i].getName();
    if (cur_filter_name == REGEX_BANNER_FILTER_NAME) {
      filters.push_back(new RegexManager(banjax_dir, main_root));
    } else if (cur_filter_name == CHALLENGER_FILTER_NAME){
      filters.push_back(new ChallengeManager(banjax_dir, main_root));
    } else if (cur_filter_name == WHITE_LISTER_FILTER_NAME){
      filters.push_back(new WhiteLister(banjax_dir, main_root));
    } else {
      //unrecognized filter, warning and pass
      TSDebug(BANJAX_PLUGIN_NAME.c_str(), "I do not recognize filter %s requested in the config", cur_filter_name.c_str());
    }
  }

}

Banjax::Banjax()
  : all_filters_requested_part(0)
{
  /* create an TSTextLogObject to log blacklisted requests to */
  TSReturnCode error = TSTextLogObjectCreate(BANJAX_PLUGIN_NAME.c_str(), TS_LOG_MODE_ADD_TIMESTAMP, &log);
  if (!log || error == TS_ERROR) {
    TSDebug(BANJAX_PLUGIN_NAME.c_str(), "error while creating log");
  }
  
  TSDebug(BANJAX_PLUGIN_NAME.c_str(), "in the beginning");

  //regex_mutex = TSMutexCreate();
  global_contp = TSContCreate(ATSEventHandler::banjax_global_eventhandler, NULL);

  BanjaxContinuation* cd = (BanjaxContinuation *) TSmalloc(sizeof(BanjaxContinuation));
  cd = new(cd) BanjaxContinuation(NULL); //no transaction attached to this cont
  TSContDataSet(global_contp, cd);

  cd->contp = global_contp;
  cd->cur_banjax_inst = this;

  TSHttpHookAdd(TS_HTTP_TXN_START_HOOK, global_contp);

  read_configuration();

  //Ask each filter what part of http transaction they are interested in
  for(list<BanjaxFilter*>::iterator cur_filter = filters.begin(); cur_filter != filters.end(); cur_filter++)
    all_filters_requested_part |= (*cur_filter)->requested_info();

}

void
Banjax::read_configuration()
{
  // Read the file. If there is an error, report it and exit.
  string sep = "/";
  string banjax_dir = TSPluginDirGet() + sep + BANJAX_PLUGIN_NAME;
  string absolute_config_file = /*TSInstallDirGet() + sep + */ banjax_dir + sep+ CONFIG_FILENAME;

  TSDebug(BANJAX_PLUGIN_NAME.c_str(), "Reading configuration from [%s]", absolute_config_file.c_str());

  try
  {
    cfg.readFile(absolute_config_file.c_str());
  }
  catch(const libconfig::FileIOException &fioex)
  {
    TSDebug(BANJAX_PLUGIN_NAME.c_str(), "I/O error while reading config file [%s].", absolute_config_file.c_str());
    return;
  }
  catch(const libconfig::ParseException &pex)
  {
    TSDebug(BANJAX_PLUGIN_NAME.c_str(), "Parse error while reading the config file");
    return;
  }

  filter_factory(banjax_dir, (const libconfig::Setting&)cfg.getRoot());

}

/* Global pointer that keep track of banjax global object */
Banjax* p_banjax_plugin;

void
TSPluginInit(int argc, const char *argv[])
{

  (void) argc; (void)argv;
  TSPluginRegistrationInfo info;

  info.plugin_name = (char*) Banjax::BANJAX_PLUGIN_NAME.c_str();
  info.vendor_name = (char*) "eQualit.ie";
  info.support_email = (char*) "info@deflect.ca";

  if (TSPluginRegister(TS_SDK_VERSION_3_0, &info) != TS_SUCCESS) {
    TSError("Plugin registration failed. \n");
  }

  if (!check_ts_version()) {
    TSError("Plugin requires Traffic Server 3.0 or later\n");
    return;
  }
  /* create the banjax object that control the whole procedure */
  p_banjax_plugin = (Banjax*)TSmalloc(sizeof(Banjax));
  p_banjax_plugin = new(p_banjax_plugin) Banjax;
}
