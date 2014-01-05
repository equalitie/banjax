#ifndef BOTBANGERAGGREGATOR_H
#define BOTBANGERAGGREGATOR_H
#include "../log_entry.h"
#include "../log_aggregator.h"
#include "../features/feature_container.h"
#include <string>
#include <vector>
#include <map>



using namespace std;

class FeatureContainer;
class Feature;
struct LogEntry;

/* Listener class, listens to events emitted from the BotBangerAggregator
 * register with RegisterEventListener
 */
class BotBangerEventListener
{
public:
	/* Fires when all features have been calculated
	 */
	virtual void OnFeatureEvent(char *key,double *features,int numFeatures)=0;
	/* Fires when an entry is evicted from the BotBangerEventListener
	 * this happens with the oldest 10%
	 */
	virtual void OnEvictEvent(string key) {UNUSED(key);} // not mandatory to implement
	virtual ~BotBangerEventListener() {;}
};

/* This class calculates all features per IP and facilitates flushing the top 10%
 * if the maximum number of entries is reached and session management
 * A model listener+configuration is implemented in BotBangerModelListener
 *
 * features and eventlisteners will be owned by this class
 */
class BotBangerAggregator:LogAggregator,FeatureProviderInterface
{
	map<string,FeatureContainer *> _map; // ip/featurecontainer mapping holds all featurevalues and data needed for calculation
	vector<pair<Feature *,int>> _features; // registered features with the index, features are owned
	int _maxFeatureNum; // the maximum index of the feature registered
	int _featureMemoryNeeded; //

	vector<BotBangerEventListener *> _eventListeners; // multiple event listeners,owned
	unsigned int _maxEntries; // maximum number of entries

	int _sessionLength; // maximum session length, after this an entry will be cleared

	void Cleanup(); // clean up all entries and listeners
	void Prune(); // flush the oldest 10%

public:
	int PredictedMemoryUsage(); // return predicted memory usage for current configuration
	BotBangerAggregator(int maxEntries,int sessionLength=1800); // constructor, max mappings and session length can be set
	virtual ~BotBangerAggregator();
	void RegisterFeature(Feature *,int index); // register a feature at index, features are owned
	void RegisterEventListener(BotBangerEventListener *l) {_eventListeners.push_back(l);} // register event listener
	virtual void Aggregate(LogEntry *); // aggregrate logentry and calculate registered features
	virtual vector<pair<Feature *,int>> &GetFeatures() {return _features;} // to give features to featurecontainer
	virtual int GetMaxFeatureIndex() {return _maxFeatureNum;} // returns last feature index
	virtual int GetFeatureSupportMemory() {return _featureMemoryNeeded;}; // returns the size of the memory needed to support calculating all features

};
#endif
