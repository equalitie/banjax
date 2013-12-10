#include <string>
#include <iostream>
#include <ios>
#include <set>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "log_entry.h"
#include "hosthitmiss/host_hit_miss_aggregator.h"
#include "hosthitmiss/host_hit_miss_feature.h"
#include "hosthitmiss/host_hit_miss_dumper.h"
#include "hosthitmiss/host_hit_miss_actions.h"
#include "botbanger/bot_banger_aggregator.h"
#include "botbanger/bot_banger_model_listener.h"

#include "features/feature_average_time_between_requests.h"
#include "features/feature_http_status_ratio.h"
#include "features/feature_average_payload_size.h"
#include "features/feature_html_to_image_ratio.h"
#include "features/feature_request_depth.h"
#include "features/feature_session_length.h"
#include "features/feature_cycling_user_agent.h"
#include "features/feature_percentage_consecutive_requests.h"
#include "features/feature_variance_request_interval.h"
#include "features/feature_request_depth_std.h"
#include "log_entry_processor.h"


LogEntryProcessor::LogEntryProcessor():
		_bbag(NULL),
		_hhmag(NULL),
		_running(false),
		_async(false),
		_processorThreadId(0),
		_output("")
{
}

LogEntryProcessor::~LogEntryProcessor()
{
	Stop(true);
	Cleanup();
}

bool LogEntryProcessor::AddLogEntry(LogEntry *le)
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

void LogEntryProcessor::HitMissSetConfig(int period,int range)
{
	if (!_running)
	{
		if (_hhmag) delete _hhmag;
		_hhmag=new HostHitMissAggregator(period,range);
	}
}

bool LogEntryProcessor::Start(bool async)
{
	if (_running) return false;
	_async=async;
	if (_async)
	{
		_running=true;
		pthread_create(&_processorThreadId,NULL,processorThread,this);
		WaitForControlAck();
	}
	_running=true;
	return true;
}

bool LogEntryProcessor::WaitForControlAck()
{
	char q=_ackQueue.Get();
	UNUSED(q);

	// maybe timeout
	/*zmq::message_t message(4);
	_zmqAckReceiver->recv(&message,0);*/
	return true;
}

bool LogEntryProcessor::ReceiveLogEntry(LogEntry *le)
{

	 FifoMessage *volatile msg=_logEntryQueue.Get();

	auto ok=(msg->size()==sizeof(LogEntry));
	if (ok)
	{
		msg->CopyMessageData(le);
	}

	msg->deleteMessage();
	return ok;
}


bool LogEntryProcessor::SendStop()
{
	/*zmq::message_t message(1);
	*((char *) message.data())=255;
	_zmqLogEntrySender->send(message,0);*/

	FifoMessage *msg=FifoMessage::create((char *) "",1);
	return _logEntryQueue.ForcedAdd(msg);

}

bool LogEntryProcessor::SendLogEntry(LogEntry *le)
{
	FifoMessage *msg=FifoMessage::create(le,sizeof(LogEntry));
	return _logEntryQueue.Add(msg);

}

bool LogEntryProcessor::SendAck()
{
	return _ackQueue.ForcedAdd('a');
	/*zmq::message_t message(4);
	strcpy(( char *) message.data(),( char*) "ack");
	_zmqAckSender->send(message);*/

}

void LogEntryProcessor::RegisterEventListener(BotBangerEventListener *l)
{
	EnsureSetup();
	_bbag->RegisterEventListener(l);
}
void LogEntryProcessor::RegisterEventListener(HostHitMissEventListener *l)
{
	EnsureSetup();
	_hhmag->RegisterEventListener(l);

}
void LogEntryProcessor::RegisterEventListener(LogEntryProcessorEventListener *l)
{
	_eventListeners.push_back(l);
}
void LogEntryProcessor::EnsureSetup()
{
	if (!_hhmag) _hhmag=new HostHitMissAggregator();
	if (!_bbag) _bbag=newBotbangerAggregator();
}


void LogEntryProcessor::Stop(bool asap)
{
	if (_running)
	{
		if (_async)
		{
			if (asap) _running=false; //any outstanding messages will be ignored
			SendStop();
			WaitForControlAck();

			void *ptr;
			pthread_join(_processorThreadId,&ptr);

		}
		_running=false;
	}
}


void LogEntryProcessor::Cleanup()
{
	for (auto i=_eventListeners.begin();i!=_eventListeners.end();i++)
	{
		delete (*i);
	}
	_eventListeners.clear();
	if (_bbag) delete _bbag;
	if (_hhmag) delete _hhmag;
	_bbag=NULL;
	_hhmag=NULL;
}


void *LogEntryProcessor::processorThread(void *arg)
{
	LogEntryProcessor *p=(LogEntryProcessor *) arg;
	return p->innerProcesserThread();
}

void *LogEntryProcessor::innerProcesserThread()
{

	SendAck(); // synchronize with main thread
	LogEntry le;
	_output.reserve(200);
	_actionList.reserve(10);
	while(ReceiveLogEntry(&le))
	{
		if (!_running) continue; // do not process messages if not running



		AggregrateLogEntry(&le);

	}
	SendAck(); // synchronize with shutdown
	return 0;
}

void LogEntryProcessor::DumpPredictedMemoryUsage()
{
	// we ignore configuration entries
	int totalsize=0;
	// queue
	int queuesize=_logEntryQueue.GetMaxSize();
	if (queuesize==0)
	{
		queuesize=1000;
		std::cout<<"Warning the logentryqueue is unbounded, this might cause problems, calc based on 1000 messages"<<endl;
	}

	{
		queuesize+=queuesize*(12+sizeof(FifoMessage)+sizeof(LogEntry));// allow for some overhead for the queue
		std::cout << "queuesize " << queuesize << " bytes" << endl;
		totalsize=queuesize;
	}
	if (_bbag)
	{
		int bbsize=_bbag->PredictedMemoryUsage();
		std::cout << "botbanger " << bbsize << " bytes" << endl;
		totalsize+=bbsize;
	}

	if (_hhmag)
	{
		int hhmsize=_hhmag->PredictedMemoryUsage();
		std::cout << "hosthitmiss " << hhmsize << " bytes" << endl;
		totalsize+=hhmsize;
	}
	std::cout << "Total size" << ((totalsize+1023)/1024) << "kilobytes" << endl;
}

bool LogEntryProcessor::AggregrateLogEntry(LogEntry *le)
{
	_output.clear();
	_actionList.clear();
	for (auto i=_eventListeners.begin();i!=_eventListeners.end();i++)
	{
		(*i)->OnLogEntryStart(le);
	}
	//_actionList.clear();

	if (_hhmag) _hhmag->Aggregate(le);
	if (_bbag) _bbag->Aggregate(le);
	for (auto i=_eventListeners.begin();i!=_eventListeners.end();i++)
	{
		(*i)->OnLogEntryEnd(le,_output,_actionList);
	}
	return true;
}
BotBangerAggregator *LogEntryProcessor::newBotbangerAggregator()
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
