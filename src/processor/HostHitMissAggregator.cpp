#include <iostream>
#include <ostream>

#include "HostHitMissAggregator.h"
#include "HostHitMissFeature.h"

using namespace std;

void HostHitMissAggregator::Aggregate(LogEntry *le)
{
	string hostname=string(le->hostname);
	auto f=_map.find(hostname);
	HostHitMissFeature *c=NULL;
	
	if (f==_map.end())
	{
		_map[hostname]=c=new HostHitMissFeature(_period,_range);
	}
	else
		c=(*f).second;

	c->Aggregrate(le);
	for(auto i=_eventListeners.begin();i!=_eventListeners.end();i++)
	{
		(*i)->OnHostHitMissEvent(le->hostname,c->CurrentHitMissRange());
	}
	
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
