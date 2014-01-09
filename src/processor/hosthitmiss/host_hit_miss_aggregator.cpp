#include <iostream>
#include <ostream>

#include "host_hit_miss_aggregator.h"
#include "host_hit_miss_feature.h"

using namespace std;

/* Process log entry and aggregrate for hit/miss/total data
 */
void HostHitMissAggregator::Aggregate(LogEntry *le)
{
	string hostname=string(le->hostname);
	auto f=_map.find(hostname);
	HostHitMissFeature *c=NULL;

	
	if (f==_map.end()) // host not found
	{
		_map[hostname]=c=new HostHitMissFeature(_period,_range); // create hosthitmiss feature
	}
	else
	{
		c=(*f).second;
	}

	c->Aggregrate(le); // process logentry
	for(auto i=_eventListeners.begin();i!=_eventListeners.end();i++) // call event listeners with current range
	{
		(*i)->OnHostHitMissEvent(le->hostname,c->CurrentHitMissRange());
	}
	
}


/* Predicted memory usage based on configuration
 */
int HostHitMissAggregator::PredictedMemoryUsage()
{
	// based on 50 hostnames
	return (
			50+
			sizeof(HostHitMissFeature)+
			sizeof(HitMissRange)*((_period/_range)+2)
			)*50;
}
void HostHitMissAggregator::Dump()
{
	auto i=_map.begin();
	while (i!=_map.end())
	{
		auto key=(*i).first;
		auto hmf=(*i).second;
		std::cout << "hostname" << key << std::endl;
		hmf->Dump();
		i++;
	}

}
