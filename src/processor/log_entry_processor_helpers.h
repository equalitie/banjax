#ifndef LOG_ENTRY_PROCESSOR_TRACE_H_
#define LOG_ENTRY_PROCESSOR_TRACE_H_

// all trace and action collector classes for use by the LogEntryProcessor

#include "utils/string_dumper.h"
#include "log_entry.h"
#include "botbanger/bot_banger_aggregator.h"
#include "botbanger/bot_banger_model_listener.h"
#include "hosthitmiss/host_hit_miss_actions.h"

/*
 * dumps the feature values
 */
class BotBangerFeatureDumper:public BotBangerEventListener, public StringDumper
{
public:
	BotBangerFeatureDumper(string &output):
		StringDumper(output)
	{
	}
	virtual void OnFeatureEvent(char *key,double *features,int numFeatures)
	{
		UNUSED(key);
		for(int i=0;i<numFeatures;i++)
		{
			char db[100];
			sprintf(db,"%f",features[i]);
			addToDump(db);
		}
	}
};

/*
 * Collects all the HostHitMiss actions and pushes them to an actionList
 * to be collected by the LogEntryProcessor
 */
class HostHitMissActionCollector:public HostHitMissActions
{
	vector<externalAction> _actionList;
public:
	HostHitMissActionCollector(vector<externalAction> &actionList):
		HostHitMissActions(),
		_actionList(actionList)
	{

	}
	virtual void OnScheduleAction(HitMissRange *hmr,string &host,string &action,string &currentaction)
	{
		UNUSED(currentaction);
		UNUSED(hmr);
		_actionList.push_back(externalAction(action,host,currentaction));
	}

};

/*
 * inherited from HostHitMissActionCollector, collects actions for output
 */
class HostHitMissActionDumper:public HostHitMissActionCollector,public StringDumper
{
public:
	HostHitMissActionDumper(string &output,vector<externalAction> &actionList):
		HostHitMissActionCollector(actionList),
		StringDumper(output)
	{
	}
	void OnScheduleAction(HitMissRange *hmr,string &host,string &action,string &currentaction)
	{
		HostHitMissActionCollector::OnScheduleAction(hmr,host,action,currentaction);
		char tbuf[8000];
		sprintf(tbuf,"%s\t%s",host.c_str(),action.c_str());
		addToDump(tbuf);
		//std::cout << time.tm_hour <<":" << time.tm_min << ":" << time.tm_sec<< "\t" << host<<"\t"<<action<<endl;
	}

};

/* used for commandline output of the LogEntryProcessor
 * gathers start data and outputs gathered data at the end of processing a LogEntry
 */
class LogEntryProcessorDumper:public LogEntryProcessorEventListener,public StringDumper
{
	bool _addLogEntry;
public:
	LogEntryProcessorDumper(string &output,bool addLogEntry):
		LogEntryProcessorEventListener(),
		StringDumper(output),
		_addLogEntry(addLogEntry)
	{

	}
	virtual void OnLogEntryStart(LogEntry *le)
	{
		if (_addLogEntry)
		{
			addToDump(le->useraddress);
			addToDump(le->hostname);
			addToDump(le->url);
		}
	}
	virtual void OnLogEntryEnd(LogEntry *le,string &output,vector<externalAction> &actionList)
	{
		UNUSED(actionList);
		if (output.empty()) return;
		char timebuf[40];
		struct tm time;
		gmtime_r(&le->endTime,&time);
		sprintf(timebuf,"%02d:%02d:%02d",time.tm_hour,time.tm_min,time.tm_sec);

		cout << timebuf << "\t" << output << endl;
	}
};

/*
 * Collects all the BotBangerModelListener actions and pushes them to an actionList
 * to be collected by the LogEntryProcessor
 */
class BotBangerActionCollector:public BotBangerModelListener
{
	vector<externalAction> &_actionList;
public:
	BotBangerActionCollector(vector<externalAction> &actionList):
		BotBangerModelListener(),
		_actionList(actionList)
	{

	}

	virtual void OnModelAction(char *key,string &modelName,string &action)
	{
		string sKey=string(key);
		_actionList.push_back(externalAction(action,sKey,modelName));
	}
	virtual ~BotBangerActionCollector()
	{

	}
};

/* inherited from BotBangerModelListener, adds additional logging
 * for tracing dependant on settings
 */
class BotBangerValueDumper:public BotBangerActionCollector,public StringDumper
{
	int _traceSetting;
public:
	BotBangerValueDumper(string &output,int traceSetting,vector<externalAction> &actionList):
		BotBangerActionCollector(actionList),
		StringDumper(output),
		_traceSetting(traceSetting)

	{
	}
	virtual void OnEvictEvent(string key)
	{
		if (_traceSetting&TraceBotBangerIPEvict)
		{
			addToDump((char *)"EVICTEDIP");
			addToDump(key.c_str());
		}

	}

	virtual void OnModelValue(char *key,string &modelName,double value)
	{
		UNUSED(key);
		if (_traceSetting&TraceBotBangerModelValues)
		{
			char buffer[140];
			sprintf(buffer,"%s\t%f",modelName.c_str(),value);
			addToDump(buffer);
		}
	}
	virtual void OnNodeValues(char *key,svm_node *values,int num)
	{
		UNUSED(key);
		if (_traceSetting&TraceBotBangerModelInputs)
		{
			for(int i=0;i<num;i++)
			{
				char db[100];
				sprintf(db,"%f",values[i].value);
				addToDump(db);
			}
		}
	}

	virtual void OnModelAction(char *key,string &modelName,string &action)
	{
		UNUSED(key);
		if (_traceSetting&TraceBotBangerAction)
		{
			char buffer[140];
			sprintf(buffer,"%s\t%s\t%s",key,modelName.c_str(),action.c_str());
			addToDump(buffer);
		}
		BotBangerActionCollector::OnModelAction(key,modelName,action);
	}

};





#endif /* LOG_ENTRY_PROCESSOR_TRACE_H_ */
