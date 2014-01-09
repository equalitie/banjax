/*
 * Tools to initiate and communicate with the LogProcessor thread, Log processor
 * is responsible to run any filter that needs to process logs from multiple requests/
 * threads
 *
 * Copyright (c) eQualit.ie 2014 under GNU AGPL V3.0 or later
 * 
 * Vmon: January 2014
 */

#include "banjax_common.h"

#include "processor/log_processor_action_2_banjax.h"
#include "processor/log_entry_processor.h"
#include "processor/log_entry_processor_config.h"

/**
     initiating the interface and the processor and set its behavoir according to
     configuration.
     
     @param cfg pointer to log_processor config of banjax.conf
*/
BanjaxLogProcessorInterface::BanjaxLogProcessorInterface(const libconfig::Setting& cfg)
{
  vector<string> warnings;
  leProcessor=new LogEntryProcessor();

  if (!LogEntryProcessorConfig::ReadFromSettings(leProcessor,&cfg,warnings,ServerMode) || warnings.size())
  {
    TSDebug(BANJAX_PLUGIN_NAME, "Failure reading settings for LogEntryProcessor");
    for(auto i=warnings.begin();i!=warnings.end();i++)
      TSDebug(BANJAX_PLUGIN_NAME,"Configuration %s",(*i).c_str());
    delete leProcessor;
    leProcessor=NULL;
    //I need to throw exception here to say that processor isn't initiated
    throw BanjaxLogProcessorInterface::CONFIG_ERROR;

  }

  leProcessor->Start(true);

}
