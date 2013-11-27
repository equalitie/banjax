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
	map<string,HostHitMissFeature*> _map;
	vector<HostHitMissEventListener *> _eventListeners;
public:
	void RegisterEventListener(HostHitMissEventListener *l) {_eventListeners.push_back(l);}
	virtual void Aggregate(LogEntry *);
	void Dump();

};
#endif