#ifndef PROCESSOR_H
#define PROCESSOR_H
#include <string>
#include <iostream>
#include <ios>
#include <fstream>
#include "ConcurrentFifo.h"
#include "LogEntry.h"
#include "HostHitMissAggregator.h"
#include "HostHitMissFeature.h"
#include "HostHitMissDumper.h"
#include "HostHitMissActions.h"
#include "BotBangerAggregrator.h"
#include "FeatureAverageTimeBetweenRequests.h"
#include "FeatureHTTPStatusRatio.h"
#include "FeatureAveragePayloadSize.h"
#include "FeatureHtmlToImageRatio.h"
#include "FeatureRequestDepth.h"
#include "FeatureSessionLength.h"
#include "FeatureCyclingUserAgent.h"
#include "FeaturePercentageConsecutiveRequests.h"
#include "FeatureVarianceRequestInterval.h"
#include "FeatureRequestDepthStd.h"
#include "BotBangerModelListener.h"
#include "svm/svm.h"
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <libconfig.h++>
#include <zmq.hpp>

#define ZMQLOGENTRYSOCKET "inproc://#1"
#define ZMQACKSOCKET "inproc://#2"

using namespace std;
using namespace libconfig;


struct externalAction
{
	string action;
	string argument1;
	string argument2;
	string argument3;
	externalAction(string &action,string &argument1,string &argument2,string &argument3):
		action(action),
		argument1(argument1),
		argument2(argument2),
		argument3(argument3)
	{

	}
	externalAction(string &action,string &argument1,string &argument2):
			action(action),
			argument1(argument1),
			argument2(argument2),
			argument3()
		{

		}


};

struct FifoMessage
{
	size_t messagesize;
	char data[1];
	void deleteMessage() {delete [] (char *) this;}
	static FifoMessage *create(void *data,size_t length)
	{
		FifoMessage *v=(FifoMessage *) new char [sizeof(size_t)+length];
		v->messagesize=length;
		memcpy(v->data,data,length);
		return v;
	}
	void CopyMessageData(void *data)
	{
		memcpy(data,this->data,messagesize);
	}
	int size() {return messagesize;}
};

class LogEntryProcessorEventListener
{
public:
	virtual void OnLogEntryStart(LogEntry *le)=0;
	virtual void OnLogEntryEnd(LogEntry *le,string &output,vector<externalAction> &actionList)=0;
	virtual ~LogEntryProcessorEventListener() {;}
};

class LogEntryProcessor
{
	BotBangerAggregator *_bbag;
	HostHitMissAggregator *_hhmag;
	HostHitMissActions *_hhmActionListener;
	volatile bool _running;
	bool _async;

	ConcurrentFifo<char> ackQueue;
	ConcurrentFifo<FifoMessage *> logEntryQueue;

	zmq::context_t *_zmqContext;

	zmq::socket_t  *_zmqLogEntrySender;
	zmq::socket_t *_zmqLogEntryReceiver;
	zmq::socket_t *_zmqAckReceiver;
	zmq::socket_t *_zmqAckSender;

	pthread_t processorThreadId;
	string output;
	vector<externalAction> actions;
	vector<LogEntryProcessorEventListener *> eventListeners;

	static void * processorThread(void *arg)
	{
		LogEntryProcessor *p=(LogEntryProcessor *) arg;
		return p->innerProcesserThread();
	}

protected:
	void *innerProcesserThread()
	{

		SendAck();
		std::cout << "started" << endl;
		LogEntry le;
		output.reserve(200);
		actions.reserve(10);
		while(ReceiveLogEntry(&le))
		{
			if (!_running) continue;



			AggregrateLogEntry(&le);

		}
		std::cout << "ended" << endl;
		SendAck();
		return 0;
	}

	bool AggregrateLogEntry(LogEntry *le)
	{
		output.clear();
		actions.clear();
		for (auto i=eventListeners.begin();i!=eventListeners.end();i++)
		{
			(*i)->OnLogEntryStart(le);
		}
		//actions.clear();

		if (_hhmag) _hhmag->Aggregate(le);
		if (_bbag) _bbag->Aggregate(le);
		for (auto i=eventListeners.begin();i!=eventListeners.end();i++)
		{
			(*i)->OnLogEntryEnd(le,output,actions);
		}
		return true;
	}
	BotBangerAggregator *newBotbangerAggregator()
	{
		auto bbag=new BotBangerAggregator(50000);
		bbag->RegisterFeature(new FeatureAverageTimeBetweenRequests(),0);//ok
		bbag->RegisterFeature(new FeatureCyclingUserAgent(),1); //nok

		bbag->RegisterFeature(new FeatureHtmlToImageRatio(),2);	//ok
		bbag->RegisterFeature(new FeatureVarianceRequestInterval(),3); //nok
		bbag->RegisterFeature(new FeatureAveragePayloadSize,4); // ok
		bbag->RegisterFeature(new FeatureHTTPStatusRatio(),5);
		bbag->RegisterFeature(new FeatureRequestDepth(),6);
		bbag->RegisterFeature(new FeatureRequestDepthStd(),7);
		bbag->RegisterFeature(new FeatureSessionLength(),8);	//ok
		bbag->RegisterFeature(new FeaturePercentageConsecutiveRequests(),9);
		return bbag;
	}

public:
	LogEntryProcessor():
		_bbag(NULL),
		_hhmag(NULL),
		_hhmActionListener(NULL),
		_running(false),
		_async(false),
		_zmqContext(NULL),
		_zmqLogEntrySender(NULL),
		_zmqLogEntryReceiver(NULL),
		_zmqAckReceiver(NULL),
		_zmqAckSender(NULL),
		processorThreadId(0)
	{
	}
	void Cleanup()
	{
		for (auto i=eventListeners.begin();i!=eventListeners.end();i++)
		{
			delete (*i);
		}
		eventListeners.clear();
		if (_bbag) delete _bbag;
		if (_hhmag) delete _hhmag;
		_bbag=NULL;
		_hhmag=NULL;
	}
	~LogEntryProcessor()
	{
		Stop(true);
		Cleanup();


	}

	bool AddLogEntry(LogEntry *le)
	{
		if (_async)
		{
			SendLogEntry(le);
		}
		else
		{
			AggregrateLogEntry(le);
		}
		return true;
	}
	void HitMissSetConfig(int period,int range)
	{
		if (!_running)
		{
			if (_hhmag) delete _hhmag;
			_hhmag=new HostHitMissAggregator(period,range);
		}
	}


	bool Start(bool async=true)
	{
		if (_running) return false;
		_async=async;
		if (_async)
		{
			_running=true;
			_zmqContext= new zmq::context_t(1);

			_zmqAckReceiver = new zmq::socket_t(*_zmqContext,ZMQ_PULL);
			_zmqAckReceiver->bind(ZMQACKSOCKET);


			_zmqLogEntryReceiver = new zmq::socket_t(*_zmqContext,ZMQ_PULL);
			_zmqLogEntryReceiver->bind(ZMQLOGENTRYSOCKET);


			_zmqAckSender = new zmq::socket_t(*_zmqContext,ZMQ_PUSH);
			_zmqAckSender->connect(ZMQACKSOCKET);


			_zmqLogEntrySender = new zmq::socket_t(*_zmqContext,ZMQ_PUSH);
			_zmqLogEntrySender->connect(ZMQLOGENTRYSOCKET);

			pthread_create(&processorThreadId,NULL,processorThread,this);

			WaitForControlAck();

		}
		_running=true;
		return true;
	}
	bool WaitForControlAck()
	{
		char q=ackQueue.Get();
		UNUSED(q);

		// maybe timeout
		/*zmq::message_t message(4);
		_zmqAckReceiver->recv(&message,0);*/
		return true;
	}

	bool ReceiveLogEntry(LogEntry *le)
	{

		zmq::message_t message(sizeof(LogEntry));

		FifoMessage *msg=logEntryQueue.Get();

		auto ok=(message.size()==sizeof(LogEntry));
		if (ok)
		{
			msg->CopyMessageData(le);
		}
		msg->deleteMessage();


		/*auto rc=_zmqLogEntryReceiver->recv(&message,0);
		if (!rc)
			std::cout << "receive error";

		else
		{
			std::cout<< "unexpected " << message.size() << endl;
		}*/
		return ok;
	}


	bool SendStop()
	{
		/*zmq::message_t message(1);
		*((char *) message.data())=255;
		_zmqLogEntrySender->send(message,0);*/

		FifoMessage *msg=FifoMessage::create((char *) "",1);
		logEntryQueue.Add(msg);
		return true;
	}

	bool SendLogEntry(LogEntry *le)
	{
		FifoMessage *msg=FifoMessage::create(le,sizeof(LogEntry));
		logEntryQueue.Add(msg);
		return true;
	}
	bool SendAck()
	{
		ackQueue.Add('a');
		/*zmq::message_t message(4);
		strcpy(( char *) message.data(),( char*) "ack");
		_zmqAckSender->send(message);*/
		return true;
	}
	void Stop(bool asap=false)
	{
		if (_running)
		{
			if (_async)
			{
				if (asap) _running=false; //any outstanding messages will be ignored
				SendStop();
				WaitForControlAck();

				void *ptr;
				pthread_join(processorThreadId,&ptr);
				if (_zmqContext)
				{
					_zmqLogEntryReceiver->close();
					delete _zmqLogEntryReceiver;

					//_zmqLogEntrySender->close();
					delete _zmqLogEntrySender;

					_zmqAckReceiver->close();
					delete _zmqAckReceiver;

					_zmqAckSender->close();
					delete _zmqAckSender;

					delete _zmqContext;
					_zmqContext=NULL;
				}
			}
			_running=false;
		}
	}

	void RegisterEventListener(BotBangerEventListener *l)
	{
		EnsureSetup();
		_bbag->RegisterEventListener(l);
	}
	void RegisterEventListener(HostHitMissEventListener *l)
	{
		EnsureSetup();
		_hhmag->RegisterEventListener(l);

	}
	void RegisterEventListener(LogEntryProcessorEventListener *l)
	{
		eventListeners.push_back(l);
	}
	void EnsureSetup()
	{
		if (!_hhmag) _hhmag=new HostHitMissAggregator();
		if (!_bbag) _bbag=newBotbangerAggregator();
	}
	friend class LogEntryProcessorConfig;
};

class BotBangerFeatureDumper:public BotBangerEventListener, public StringDumper
{
public:
	BotBangerFeatureDumper(string &output):
		StringDumper(output)
	{
	}
	virtual void OnFeatureEvent(char *key,float *features,int numFeatures)
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

enum LogEntryTrace {
	TraceHitMissAction=1,
	TraceBotBangerAction=2,
	TraceHitMissRatio=4,
	TraceBotBangerFeatures=8,
	TraceBotBangerModelValues=16,
	TraceBotBangerModelInputs=32,
	TraceLogEntries=64,
	ConsoleMode=128,
	ServerMode=256
	};


class HostHitMissActionCollector:public HostHitMissActions
{
	vector<externalAction> _actionList;
public:
	HostHitMissActionCollector(vector<externalAction> &actionList):
		HostHitMissActions(),
		_actionList(actionList)
	{

	}
	virtual void ScheduleAction(HitMissRange *hmr,string &host,string &action,string &currentaction)
	{
		UNUSED(currentaction);
		UNUSED(hmr);
		_actionList.push_back(externalAction(action,host,currentaction));
	}

};

class HostHitMissActionDumper:public HostHitMissActionCollector,public StringDumper
{
public:
	HostHitMissActionDumper(string &output,vector<externalAction> &actionList):
		HostHitMissActionCollector(actionList),
		StringDumper(output)
	{
	}
	void ScheduleAction(HitMissRange *hmr,string &host,string &action,string &currentaction)
	{
		UNUSED(hmr);
		char tbuf[8000];
		if (currentaction==action) return;


		sprintf(tbuf,"%s\t%s",host.c_str(),action.c_str());
		addToDump(tbuf);

		//std::cout << time.tm_hour <<":" << time.tm_min << ":" << time.tm_sec<< "\t" << host<<"\t"<<action<<endl;
	}

};

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
		sprintf(timebuf,"%02d:%02d:%02dx",time.tm_hour,time.tm_min,time.tm_sec);

		cout << timebuf << "\t" << output << endl;
	}
};


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



class LogEntryProcessorConfig
{
public:
	static bool ReadFromSettings(LogEntryProcessor *lp,string configfile,vector<string> &warnings, int consoleSettings=0)
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
	static bool ReadFromSettings(LogEntryProcessor *lp,Config *configuration,vector<string> &warnings,int consoleSettings)
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
	static bool ReadFromSettings(LogEntryProcessor *lp,Setting *hitMissSettings,Setting *botBangerSettings,vector<string> &warnings, int consoleSettings=0)
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
				lp->RegisterEventListener(new HostHitMissDumper(lp->output, consoleSettings^(TraceHitMissRatio|ConsoleMode|ServerMode)));
			}
			if (consoleSettings&TraceHitMissAction)
			{
				hmac=new HostHitMissActionDumper(lp->output,lp->actions);
				lp->RegisterEventListener(hmac);
			}
			if (consoleSettings&TraceBotBangerFeatures)
			{
				lp->RegisterEventListener(new BotBangerFeatureDumper(lp->output));
			}
			if (consoleSettings&(TraceBotBangerModelValues|TraceBotBangerModelInputs|TraceBotBangerAction))
			{
				bbml=new BotBangerValueDumper(lp->output,consoleSettings,lp->actions);
				lp->RegisterEventListener(bbml);
			}
			if (consoleSettings&ConsoleMode)
			{
				lp->RegisterEventListener(new LogEntryProcessorDumper(lp->output,consoleSettings&TraceLogEntries));
			}
			if (consoleSettings&ServerMode)
			{
				if (hitMissSettings && !hmac)
				{
					hmac=new HostHitMissActionCollector(lp->actions);
					lp->RegisterEventListener(hmac);
				}
				if (botBangerSettings && !bbml)
				{
					bbml=new BotBangerActionCollector(lp->actions);
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
}
;
#endif
