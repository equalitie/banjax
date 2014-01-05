#include "log_entry_processor_config.h"
#include "hosthitmiss/host_hit_miss_dumper.h"
#include "log_entry_processor_helpers.h"
using namespace libconfig;
using namespace std;


/* configure a LogProcessor from a configfile, consolesettings is a bitmask of LogEntryTrace entries */
bool LogEntryProcessorConfig::ReadFromSettings(LogEntryProcessor *lp,string configfile,vector<string> &warnings, int consoleSettings)
{
	try
	{
		Config config;
		config.readFile((const char*) (configfile.c_str()));
		return ReadFromSettings(lp,&config,warnings,consoleSettings);
		/*Setting *hitMissSettings=NULL;
		try
		{
			&(config.lookup("hitmiss"));
		}
		Setting &botBangerSettings=config.lookup("bot_banger");
		return ReadFromSettings(lp,hitMissSettings,&botBangerSettings,warnings,consoleSettings);*/
	}
	catch(ParseException &pex)
	{
		char buffer[100];
		sprintf(buffer,"%d",pex.getLine());

		warnings.push_back((string(pex.what())+" at line :")+buffer);
		return false;
	}
	catch(ConfigException &err)
	{
		warnings.push_back(string(err.what()));
		return false;
	}
	return true;



}
/* configure a LogProcessor from a libconfig::Config, consolesettings is a bitmask of LogEntryTrace entries */
bool LogEntryProcessorConfig::ReadFromSettings(LogEntryProcessor *lp,Config *configuration,vector<string> &warnings,int consoleSettings)
{
	return ReadFromSettings(lp,configuration->getRoot(),warnings,consoleSettings);
}
/* configure a LogProcessor from a libconfig::Config, consolesettings is a bitmask of LogEntryTrace entries */
bool LogEntryProcessorConfig::ReadFromSettings(LogEntryProcessor *lp,Setting *configuration,vector<string> &warnings,int consoleSettings)
{
	try
	{

		Setting *hitMissSettings=NULL;
		try
		{
			hitMissSettings=&((*configuration )["hitmiss"]);
		}
		catch(SettingNotFoundException err)
		{
			//ignore
		}
		Setting *botBangerSettings=NULL;
		try
		{
			botBangerSettings=&((*configuration )["bot_banger"]);
		}
		catch(SettingNotFoundException err)
		{
			//ignore
		}
		return ReadFromSettings(lp,hitMissSettings,botBangerSettings,warnings,consoleSettings);
	}
	catch(SettingException &err)
	{
		warnings.push_back(string(err.getPath()+string(" ")+err.what()));
		return false;
	}
}

/* setup hitmiss */
bool LogEntryProcessorConfig::setupHitMiss(LogEntryProcessor *lp,Setting *hitMissSettings,vector<string> &warnings, int &consoleSettings)
{
	HostHitMissActions *hmac=NULL;
	if (!hitMissSettings) return true;
	if (!consoleSettings)
	{
		warnings.push_back(string("No consolemode set"));
		return false;
	}

	int period;
	int range;
	int traceFlags=0;

	if (!hitMissSettings->lookupValue("period",period)) period=60;
	if (!hitMissSettings->lookupValue("range",range)) range=5;
	if (hitMissSettings->lookupValue("trace_flags",traceFlags))
	{
		consoleSettings|= // we also change this for the caller
				(
						traceFlags&  // take only the relevant flags
						(
								TraceHitMissAction|
								TraceHitMissRatio|
								TraceLogEntries
						)
				);
	}
	lp->HitMissSetConfig(period,range);
	if (consoleSettings)
	{
		if (consoleSettings&TraceHitMissRatio) // add a listener for the hitmiss ratio
		{
			lp->RegisterEventListener(new HostHitMissDumper(lp->_output, consoleSettings^(TraceHitMissRatio|ConsoleMode|ServerMode)));
		}
		if (consoleSettings&TraceHitMissAction) // add a listener for the actions generated
		{
			hmac=new HostHitMissActionDumper(lp->_output,lp->_actionList); // is inherited from HostHitMissActionCollector
			lp->RegisterEventListener(hmac);
		}
		if (consoleSettings&ServerMode)
		{
			if (hitMissSettings && !hmac) // in server mode add a listener for the actions
			{
				hmac=new HostHitMissActionCollector(lp->_actionList);
				lp->RegisterEventListener(hmac);
			}
		}
	}

	if (hmac)
	{
		Setting &configs=(*hitMissSettings)["configurations"];
		for (int i=0;i<configs.getLength();i++)
		{
			Setting &s=configs[i];

			try
			{
				string host=s["host"];
				string action=s["action"];
				int requestLower=s["request_lower"];
				int requestUpper=s["request_upper"];
				double ratioLower=s["ratio_lower"];
				double ratioUpper=s["ratio_upper"];
				int runTime=s["runtime"];


				hmac->AddConfigLine(host,action,requestLower,requestUpper,ratioLower,ratioUpper,runTime);
			}
			catch(SettingException &cerr)
			{
				warnings.push_back(string(cerr.getPath())+":"+string(cerr.what()));
			}
		}
	}

	return true;
}

bool LogEntryProcessorConfig::setupBotBanger(LogEntryProcessor *lp,Setting *botBangerSettings,vector<string> &warnings, int &consoleSettings)
{
	BotBangerModelListener *bbml=NULL;
	if (!botBangerSettings) return true;
	if (!consoleSettings)
	{
		warnings.push_back(string("No consolemode set"));
		return false;
	}

	int maxips;
	int traceFlags=0;
	if (!botBangerSettings->lookupValue("max_ips",maxips) && maxips<100 && maxips>300000) maxips=10000;
	if (botBangerSettings->lookupValue("trace_flags",traceFlags))
	{
		consoleSettings|=
				(
						traceFlags&
						(
								TraceBotBangerAction|
								TraceBotBangerFeatures|
								TraceBotBangerModelInputs|
								TraceBotBangerModelValues
						)
				);
	}
	lp->BotBangerSetConfig(maxips);



	if (consoleSettings&TraceBotBangerFeatures)
	{
		lp->RegisterEventListener(new BotBangerFeatureDumper(lp->_output));
	}
	if (consoleSettings&(TraceBotBangerModelValues|TraceBotBangerModelInputs|TraceBotBangerAction))
	{
		bbml=new BotBangerValueDumper(lp->_output,consoleSettings,lp->_actionList); // inherited from BotBangerActionCollector
		lp->RegisterEventListener(bbml);
	}
	if (consoleSettings&ServerMode)
	{
		if (botBangerSettings && !bbml)
		{
			bbml=new BotBangerActionCollector(lp->_actionList);
			lp->RegisterEventListener(bbml);
		}
	}

	if (bbml)
	{
		Setting &configs=(*botBangerSettings)["configurations"];
		for (int i=0;i<configs.getLength();i++)
		{
			Setting &current=configs[i];
			string modelFilename;
			string modelName;


			if (current.lookupValue("filename",modelFilename)
					&&
				current.lookupValue("name",modelName))
			{
				svm_model *model=NULL;
				Setting &actions=current["actions"];
				for(int i=0;i<actions.getLength();i++)
				{
					try
					{
						Setting &currentAction=actions[i];
						double valueLower=currentAction["value_lower"];
						double valueUpper=currentAction["value_upper"];
						string action=currentAction["action"];

						if (!model)
						{
							model=svm_load_model(modelFilename.c_str());
							if (!model)
							{
								warnings.push_back(string("model")+modelFilename+" could not be read");
								return false;
							}
						}
						bbml->AddConfigLine(modelName,model,valueLower,valueUpper,action);
					}
					catch(SettingException &cerr)
					{
						warnings.push_back(string(cerr.getPath())+":"+string(cerr.what()));
					}

				}
			}
			else
			{
				warnings.push_back(string("incomplete botbanger configuration line"));
			}
		}
	}
	return true;

}

/* configure a LogProcessor from a libconfig::Settings, consolesettings is a bitmask of LogEntryTrace entries */
bool LogEntryProcessorConfig::ReadFromSettings(LogEntryProcessor *lp,Setting *hitMissSettings,Setting *botBangerSettings,vector<string> &warnings, int consoleSettings)
{
	UNUSED(botBangerSettings);


	lp->Cleanup();
	if (!setupHitMiss(lp,hitMissSettings,warnings,consoleSettings)) return false;
	if (!setupBotBanger(lp,botBangerSettings,warnings,consoleSettings)) return false;




	if (consoleSettings)
	{
		if (consoleSettings&ConsoleMode)
		{
			lp->RegisterEventListener(new LogEntryProcessorDumper(lp->_output,consoleSettings&TraceLogEntries));
		}
	}
	else
	{
		warnings.push_back(string("No consolemode set"));
		return false;
	}

	return true;

}
