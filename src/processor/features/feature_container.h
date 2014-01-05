#ifndef FEATURECONTAINER_H
#define FEATURECONTAINER_H
#include <algorithm>
#include <time.h>
#include <vector>
#include <string.h>
#include "../log_entry.h"
#include "feature.h"





using namespace std;
struct LogEntry;
class Feature;

/* This class decouples feature registration from the feature container
 *
 * Inherited by:
 * BotBangerAggregator to setup
 */
class FeatureProviderInterface
{
public:
	/* get a reference to the Feature array, each entry has  */
	virtual vector<pair<Feature *,int>> &GetFeatures()=0;
	/* max feature index*/
	virtual int GetMaxFeatureIndex()=0;
	/* get the memory in bytes needed to store all feature data */
	virtual int GetFeatureSupportMemory()=0;
	virtual ~FeatureProviderInterface() {;}
};

/* A featurecontainer holds all features and necessary memory for the features
 * configuration in the FeatureProvider used in the construction
 *
 * Used by:
 * BotBangerAggregator for each IP
 *
 * Inherited by:
 * FeatureProviderTest to setup a test harness for a feature
 *
 */
class FeatureContainer
{
	vector<pair<Feature *,int>> &_features; // reference to the features with the feature index provided
	double *_featureData; // pointer to the feature array, owned
	char *_data; // pointer to support memory allocated for the features
	int _memlen; // memory length for feature support memory and features

public:
	/* Constructor, takes a FeatureProviderInterface which has the configuration
	 * allocates,zeroes memory and sets up pointers
	 */
	FeatureContainer(FeatureProviderInterface *fpi):
		_features(fpi->GetFeatures()),
		numRequests(0)
	{
		// we make the feature container responsible for calculating the memory needed
		_data=new char[_memlen=(
				fpi->GetFeatureSupportMemory()+  // feature support memory
				(fpi->GetMaxFeatureIndex()+1)*sizeof(double)) // feature data
		               ];
		memset(_data,0,_memlen);
		_featureData=(double *)(_data+fpi->GetFeatureSupportMemory()); // we will store the features after the memory used for the features


		firstRequestTime=0;
		lastRequestTime=0;


	}

	/* Get a pointer to the feature array */
	double *GetFeatureData()
	{
		return _featureData;
	}
	/* destructor, delete feature array+ feature support memory */
	virtual ~FeatureContainer()
	{
		delete [] _data;
	}

	/* First request time has the first request time seen, not the lowest
	 * because we want to prevent that features will be screwed up
	 */
	time_t firstRequestTime;

	/* Last request time has the maximal request time seen, not the last
	 * because we want to prevent that features will be screwed up
	 */
	time_t lastRequestTime;

	/* Number of requests seen */
	int numRequests;

	/* clear all data, for example after a session has ended */
	void Clear()
	{
		memset(_data,0,_memlen);
		firstRequestTime=0;
		lastRequestTime=0;
		numRequests=0;
	}

	/* Aggregrate a logentry and pass it to all features */
	virtual void Aggregrate(LogEntry *le)
	{
		// adjust local data
		if (!firstRequestTime) firstRequestTime=le->endTime;
		lastRequestTime=max(lastRequestTime,le->endTime);
		numRequests++;

		// adjust features
		char *dataOffset=_data; // where the data for the feature lives
		for(auto i=_features.begin();i!=_features.end();i++) // loop the features
		{
			auto f=*i; // f.first is a feature, f.second is the index
			f.first->Aggregrate(le,this,dataOffset,_featureData+f.second);
			dataOffset+=f.first->GetDataSize(); // move pointer to next feature data
		}
	}
};
#endif
