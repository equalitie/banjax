#ifndef PROCESSOR_H
#define PROCESSOR_H




#include "utils/concurrent_fifo.h"
#include "utils/svm/svm.h"
#include "external_action.h"


struct LogEntry;
class BotBangerAggregator;
class BotBangerEventListener;
class HostHitMissAggregator;
class HostHitMissEventListener;

using namespace std;


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

	volatile bool _running;
	bool _async;

	ConcurrentFifo<char> _ackQueue;
	ConcurrentFifo<FifoMessage *> _logEntryQueue;

	pthread_t _processorThreadId;
	string _output;
	vector<externalAction> _actionList;
	vector<LogEntryProcessorEventListener *> _eventListeners;

	static void * processorThread(void *arg);
	void *innerProcesserThread();
	bool AggregrateLogEntry(LogEntry *le);

	bool WaitForControlAck();
	bool ReceiveLogEntry(LogEntry *le);
	bool SendStop();
	bool SendLogEntry(LogEntry *le);
	bool SendAck();
	void Cleanup();
	void EnsureSetup();


	BotBangerAggregator *newBotbangerAggregator();

protected:


public:
	LogEntryProcessor();

	~LogEntryProcessor();


	bool AddLogEntry(LogEntry *le);

	void HitMissSetConfig(int period,int range);
	void DumpPredictedMemoryUsage();

	bool Start(bool async=true);
	void Stop(bool asap=false);
	void RegisterEventListener(BotBangerEventListener *l);
	void RegisterEventListener(HostHitMissEventListener *l);
	void RegisterEventListener(LogEntryProcessorEventListener *l);


	friend class LogEntryProcessorConfig;
};

#endif
