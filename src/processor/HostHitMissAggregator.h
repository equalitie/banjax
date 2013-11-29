#ifndef HOSTHITMISSAGGREGATOR_H
#define HOSTHITMISSAGGREGATOR_H

#include "LogEntry.h"
#include "LogAggregator.h"
#include "HostHitMissFeature.h"
#include <string>
#include <vector>
#include <map>


using namespace std;

class FeatureContainer;
class HostHitMissFeature;
struct LogEntry;

class HostHitMissEventListener
{
public:
	virtual void OnHostHitMissEvent(char *host,HitMissRange *hmrange)=0;
};

class HostHitMissAggregator:LogAggregator
{
	int _period,_range;
	map<string,HostHitMissFeature*> _map;
	vector<HostHitMissEventListener *> _eventListeners;
public:
	HostHitMissAggregator(int period=60,int range=5):
		_period(period),
		_range(range)
	{

	}
	void RegisterEventListener(HostHitMissEventListener *l) {_eventListeners.push_back(l);}
	virtual void Aggregate(LogEntry *);
	void Dump();
	virtual ~HostHitMissAggregator() {;}// TODO: cleanup

};
#endif
