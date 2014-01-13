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

#include "../include/log_processor_interface.h"
#include "processor/log_entry_processor.h"
#include "processor/log_entry_processor_config.h"
#include "processor/log_entry.h"

/**
     initiating the interface and the processor and set its behavoir according to
     configuration.
     
     @param cfg pointer to log_processor config of banjax.conf
*/
BanjaxLogProcessorInterface::BanjaxLogProcessorInterface(const libconfig::Setting& cfg)
:LogEntryProcessorEventListener()
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
  // we are listening to what the LogEntryProcessor says
  leProcessor->RegisterEventListener(this);

  leProcessor->Start(true);

}

void BanjaxLogProcessorInterface::OnLogEntryStart(LogEntry *le)
{
	UNUSED(le);
}
void BanjaxLogProcessorInterface::OnLogEntryEnd(LogEntry *le,string &output,vector<externalAction> &actionList)
{

	UNUSED(le);
	UNUSED(output);
	if (!output.empty())
	{
		struct tm time;
		gmtime_r(&le->endTime,&time);
		TSDebug(BANJAX_PLUGIN_NAME,"LPD: %02d:%02d:%02d\t%s",time.tm_hour,time.tm_min,time.tm_sec,output.c_str());
	}

	for(auto i=actionList.begin();i!=actionList.end();i++)
	{
		externalAction &ea=(*i);
		TSDebug(BANJAX_PLUGIN_NAME,"LPH: %s/%s/%s/%s",ea.action.c_str(),ea.argument1.c_str(),ea.argument2.c_str(),ea.argument3.c_str());

		// running the actions in banjax
		if ((*i).action=="blockip")
		{
			// Todo: VMON, please create an integration point for this one
			// as this class does not have a pointer to banjax it does
			// not know yet how to get to banip

			//_banjax->BanIP((*i).argument1,string("banned from botbanger, model:")+(*i).argument2);
		}
		if ((*i).action=="captcha")
		{
			// how can we start a captcha for a host???
			//_banjax->StartCaptchaForHost((*i).argument1);
		}
	}


}
// this stops the LogEntryProcessor from deleting us
// it doesn't own us, we own the LogEntryProcessor
bool BanjaxLogProcessorInterface::IsOwnedByProcessor() {
	return false;
}
BanjaxLogProcessorInterface::~BanjaxLogProcessorInterface()
{
	if (leProcessor)
	{
		leProcessor->Stop();


		delete leProcessor;
	}
}

