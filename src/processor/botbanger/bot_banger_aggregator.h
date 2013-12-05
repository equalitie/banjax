#ifndef BOTBANGERAGGREGATOR_H
#define BOTBANGERAGGREGATOR_H
#include "../log_entry.h"
#include "../log_aggregator.h"
#include "../features/feature_container.h"
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
	virtual void OnFeatureEvent(char *key,double *features,int numFeatures)=0;
	virtual void OnEvictEvent(string key) {UNUSED(key);} // not mandatory to implement
	virtual ~BotBangerEventListener() {;}
};

class BotBangerAggregator:LogAggregator,FeatureProviderInterface
{
	map<string,FeatureContainer *> _map;
	vector<pair<Feature *,int>> _features;
	int _maxFeatureNum;
	int _featureMemoryNeeded;

	vector<BotBangerEventListener *> _eventListeners;
	unsigned int _maxEntries;

	int _sessionLength;

	void Cleanup();
	void Prune();

public:
	BotBangerAggregator(int maxEntries,int sessionLength=1800);
	virtual ~BotBangerAggregator();
	void RegisterFeature(Feature *,int index);
	void RegisterEventListener(BotBangerEventListener *l) {_eventListeners.push_back(l);}
	virtual void Aggregate(LogEntry *);
	virtual vector<pair<Feature *,int>> &GetFeatures() {return _features;}
	virtual int GetMaxFeatureIndex() {return _maxFeatureNum;}
	virtual int GetMemoryNeeded() {return _featureMemoryNeeded+_maxFeatureNum*sizeof(double);};

};
#endif
