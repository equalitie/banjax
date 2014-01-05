#ifndef HOSTHITMISSAGGREGATOR_H
#define HOSTHITMISSAGGREGATOR_H

#include "../log_entry.h"
#include "../log_aggregator.h"
#include "host_hit_miss_feature.h"
#include <string>
#include <vector>
#include <map>


using namespace std;

class FeatureContainer;
class HostHitMissFeature;
struct LogEntry;

/* event listener for HostHitMissAggregator */
class HostHitMissEventListener
{
public:
	/* after processing a logentry this will be called with the current HitMissRange */
	virtual void OnHostHitMissEvent(char *host,HitMissRange *hmrange)=0;
	virtual ~HostHitMissEventListener() {;}
};


/*
 * This class creates a map and distributes the hit/misses per host name
 * it uses the HostHitMissFeature to run a sliding window of range over a period.
 */
class HostHitMissAggregator:LogAggregator
{
	int _period,_range; // period is the period with several ranges of length range
	map<string,HostHitMissFeature*> _map; // mapping of hosts to the HostHitMissFeature, owned
	vector<HostHitMissEventListener *> _eventListeners; // event listeners , owned
public:
	/* constructor, period is the period measured with several ranges of length range
	 */
	HostHitMissAggregator(int period=60,int range=5):
		_period(period),
		_range(range)
	{

	}
	/* Predicted memory usage based on configuration
	 */
	int PredictedMemoryUsage();
	/* Register event listeners, owned
	 */
	void RegisterEventListener(HostHitMissEventListener *l) {_eventListeners.push_back(l);}
	/* Process log entry and aggregrate for hit/miss/total data
	 */
	virtual void Aggregate(LogEntry *);

	void Dump();

	/* cleanup
	 */
	virtual ~HostHitMissAggregator() {

		for (auto i=_eventListeners.begin();i<_eventListeners.end();i++)
		{
			delete (*i);
		}

		for(auto i=_map.begin();i!=_map.end();i++)
		{
			delete (*i).second;
		}
	}

};
#endif
