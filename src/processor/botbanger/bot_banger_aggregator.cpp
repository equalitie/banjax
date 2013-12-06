#include "../log_entry.h"
#include "../features/feature.h"
#include "../features/feature_container.h"
#include "bot_banger_aggregator.h"
#include <algorithm>




void BotBangerAggregator::RegisterFeature(Feature *f,int index)
{
	_features.push_back(pair<Feature *,int>(f,index));
	_maxFeatureNum=max(index+1,_maxFeatureNum);
	_featureMemoryNeeded+=f->GetDataSize();
}
BotBangerAggregator::BotBangerAggregator(int maxEntries,int sessionLength):
	LogAggregator(),
	_maxFeatureNum(-1),
	_featureMemoryNeeded(0),
	_maxEntries(maxEntries),
	_sessionLength(sessionLength)
{

}

void BotBangerAggregator::Cleanup()
{
	// delete eventlisteners
	for (auto i=_eventListeners.begin();i!=_eventListeners.end();i++) delete (*i);
	// delete features
	for (auto i=_features.begin();i!=_features.end();i++) delete (*i).first;
	// delete feature values
	for (auto i=_map.begin();i!=_map.end();i++) delete (*i).second;	
}

BotBangerAggregator::~BotBangerAggregator()
{
	Cleanup();
}

bool SortAges(pair<string,time_t> a,pair<string,time_t> b)
{
	return a.second<b.second; // we want a reverse sort
}

void BotBangerAggregator::Prune() {
	// prune 10%, we should never hit this, but if we do
	// it will not be a problem that it is not the fastest way
	// if this becomes a problem we need a proper LRU structure
	// there is one included in lru_cache_using_std
	// we would need to switch to scoped ptrs for that
	vector<pair<string, time_t> > ages;
	ages.reserve(_map.size());
	for (auto i = _map.begin(); i != _map.end(); i++) {
		ages.push_back(
				pair<string, time_t>((*i).first, (*i).second->lastRequestTime));
	}
	std::sort(ages.begin(), ages.end(), SortAges); // qsort , thus quick
	int prune = _map.size() / 10;
	for (auto i = ages.begin(); i != ages.end() && prune != 0; i++, prune--) {
		auto f = _map.find((*i).first);
		delete (*f).second;
		_map.erase(f);
		for(auto i=_eventListeners.begin();i!=_eventListeners.end();i++)
		{
			(*i)->OnEvictEvent((*f).first);
		}
	}
}

void BotBangerAggregator::Aggregate(LogEntry * le)
{
	
	string useraddress=string(le->useraddress);
	auto f=_map.find(useraddress);
	FeatureContainer *c=NULL;
	
	if (f!=_map.end())
	{
		c=(*f).second;
		if ((le->endTime-_sessionLength)>c->lastRequestTime) // check session timeout
			c->Clear();
	}
	else
	{
		if (_map.size()>=_maxEntries)
		{
			Prune();
		}
		_map[useraddress]=c=new FeatureContainer(this);
	}

	c->Aggregrate(le);
	
	// pass onto listeners
	for(auto i=_eventListeners.begin();i!=_eventListeners.end();i++)
	{
		(*i)->OnFeatureEvent(le->useraddress,c->GetFeatureData(),this->GetMaxFeatureIndex());
	}

}
