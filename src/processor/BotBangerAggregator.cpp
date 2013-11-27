#include "LogEntry.h"
#include "Feature.h"
#include "FeatureContainer.h"
#include "BotBangerAggregrator.h"



void BotBangerAggregator::RegisterFeature(Feature *f,int index)
{
	_features.push_back(pair<Feature *,int>(f,index));
	_maxFeatureNum=max(index+1,_maxFeatureNum);
	_featureMemoryNeeded+=f->GetDataSize();
}
BotBangerAggregator::BotBangerAggregator():
	LogAggregator(),
	_maxFeatureNum(-1),
	_featureMemoryNeeded(0)
	
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

void BotBangerAggregator::Aggregate(LogEntry * le)
{
	
	string useraddress=string(le->useraddress);
	auto f=_map.find(useraddress);
	FeatureContainer *c=NULL;
	
	if (f==_map.end())
	{
		if (_map.size()>max_mappings) // prune 10%
		{

		}
		_map[useraddress]=c=new FeatureContainer(this);
	}
	else
		c=(*f).second;

	c->Aggregrate(le);
	
	// pass onto listeners
	for(auto i=_eventListeners.begin();i!=_eventListeners.end();i++)
	{
		(*i)->OnFeatureEvent(le->useraddress,c->GetFeatureData(),this->GetMaxFeatureIndex());
	}

}
