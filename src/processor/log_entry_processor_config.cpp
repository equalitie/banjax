#include "log_entry_processor_config.h"
#include "hosthitmiss/host_hit_miss_dumper.h"
#include "log_entry_processor_helpers.h"
using namespace libconfig;
using namespace std;

bool LogEntryProcessorConfig::ReadFromSettings(LogEntryProcessor *lp,string configfile,vector<string> &warnings, int consoleSettings)
{
	try
	{
		Config config;
		config.readFile((const char*) (configfile.c_str()));
		Setting &hitMissSettings=config.lookup("hitmiss");
		Setting &botBangerSettings=config.lookup("bot_banger");
		return ReadFromSettings(lp,&hitMissSettings,&botBangerSettings,warnings,consoleSettings);
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
bool LogEntryProcessorConfig::ReadFromSettings(LogEntryProcessor *lp,Config *configuration,vector<string> &warnings,int consoleSettings)
{
	try
	{

		Setting &hitMissSettings=configuration->lookup("hitmiss");
		Setting &botBangerSettings=configuration->lookup("bot_banger");
		return ReadFromSettings(lp,&hitMissSettings,&botBangerSettings,warnings,consoleSettings);
	}
	catch(SettingException &err)
	{
		warnings.push_back(string(err.getPath()+string(" ")+err.what()));
		return false;
	}
}
bool LogEntryProcessorConfig::ReadFromSettings(LogEntryProcessor *lp,Setting *hitMissSettings,Setting *botBangerSettings,vector<string> &warnings, int consoleSettings)
{
	UNUSED(botBangerSettings);

	HostHitMissActions *hmac=NULL;
	BotBangerModelListener *bbml=NULL;
	lp->Cleanup();

	if (hitMissSettings)
	{
		int period;
		int range;
		int traceFlags=0;
		if (!hitMissSettings->lookupValue("period",period)) period=60;
		if (!hitMissSettings->lookupValue("range",range)) range=5;
		if (hitMissSettings->lookupValue("trace_flags",traceFlags))
		{
			consoleSettings|=
					(
							traceFlags&
							(
									TraceHitMissAction|
									TraceHitMissRatio|
									TraceLogEntries
							)
					);
		}
		lp->HitMissSetConfig(period,range);
	}
	if (botBangerSettings)
	{
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
		//lp->BotBangerConfig(maxips);
	}

	if (consoleSettings)
	{

		if (consoleSettings&TraceHitMissRatio)
		{
			lp->RegisterEventListener(new HostHitMissDumper(lp->_output, consoleSettings^(TraceHitMissRatio|ConsoleMode|ServerMode)));
		}
		if (consoleSettings&TraceHitMissAction)
		{
			hmac=new HostHitMissActionDumper(lp->_output,lp->_actionList);
			lp->RegisterEventListener(hmac);
		}
		if (consoleSettings&TraceBotBangerFeatures)
		{
			lp->RegisterEventListener(new BotBangerFeatureDumper(lp->_output));
		}
		if (consoleSettings&(TraceBotBangerModelValues|TraceBotBangerModelInputs|TraceBotBangerAction))
		{
			bbml=new BotBangerValueDumper(lp->_output,consoleSettings,lp->_actionList);
			lp->RegisterEventListener(bbml);
		}
		if (consoleSettings&ConsoleMode)
		{
			lp->RegisterEventListener(new LogEntryProcessorDumper(lp->_output,consoleSettings&TraceLogEntries));
		}
		if (consoleSettings&ServerMode)
		{
			if (hitMissSettings && !hmac)
			{
				hmac=new HostHitMissActionCollector(lp->_actionList);
				lp->RegisterEventListener(hmac);
			}
			if (botBangerSettings && !bbml)
			{
				bbml=new BotBangerActionCollector(lp->_actionList);
				lp->RegisterEventListener(bbml);
			}
		}
	}
	else
	{
		warnings.push_back(string("No consolemode set"));
		return false;
	}


	if (hitMissSettings && hmac)
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
	if (botBangerSettings && bbml)
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
