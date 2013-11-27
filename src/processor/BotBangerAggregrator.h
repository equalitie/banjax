#ifndef BOTBANGERAGGREGATOR_H
#define BOTBANGERAGGREGATOR_H
#include "LogEntry.h"
#include "LogAggregator.h"
#include "FeatureContainer.h"
#include <string>
#include <vector>
#include <map>


static const unsigned int max_mappings=4000;
using namespace std;


class FeatureContainer;
class Feature;

struct LogEntry;

class BotBangerEventListener
{
public:
	virtual void OnFeatureEvent(char *key,float *features,int numFeatures)=0;	
	virtual ~BotBangerEventListener() {;}
};

class BotBangerAggregator:LogAggregator,FeatureProviderInterface
{
	map<string,FeatureContainer *> _map;
	vector<pair<Feature *,int>> _features;
	int _maxFeatureNum;
	int _featureMemoryNeeded;

	vector<BotBangerEventListener *> _eventListeners;
	void Cleanup();
public:
	BotBangerAggregator();
	virtual ~BotBangerAggregator();
	void RegisterFeature(Feature *,int index);
	void RegisterEventListener(BotBangerEventListener *l) {_eventListeners.push_back(l);}
	virtual void Aggregate(LogEntry *);
	virtual vector<pair<Feature *,int>> &GetFeatures() {return _features;}
	virtual int GetMaxFeatureIndex() {return _maxFeatureNum;}
	virtual int GetMemoryNeeded() {return _featureMemoryNeeded+_maxFeatureNum*sizeof(float);};

};
#endif
