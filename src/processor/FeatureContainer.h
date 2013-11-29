#ifndef FEATURECONTAINER_H
#define FEATURECONTAINER_H
#include <algorithm>
#include <time.h>
#include <vector>
#include <string.h>
#include "Feature.h"


#define MAX_IDEAL_SESSION_LENGTH 1800
#define SESSION_EXPIRATION_TIME	1800


using namespace std;
struct LogEntry;
class Feature;

class FeatureProviderInterface
{
public:
	virtual vector<pair<Feature *,int>> &GetFeatures()=0;
	virtual int GetMaxFeatureIndex()=0;
	virtual int GetMemoryNeeded()=0;
};

class FeatureContainer
{	
	vector<pair<Feature *,int>> &_features;
	float *_featureData;	
	char *_data;
	int _memlen;
public:
	FeatureContainer(FeatureProviderInterface *fpi):
		_features(fpi->GetFeatures())
	{
		_data=new char[_memlen=fpi->GetMemoryNeeded()];
		memset(_data,0,_memlen);
		firstRequestTime=0;
		lastRequestTime=0;
		_featureData=(float *)(_data+fpi->GetMemoryNeeded()-fpi->GetMaxFeatureIndex()*sizeof(float));		
	}

	float *GetFeatureData()
	{
		return _featureData;
	}
	virtual ~FeatureContainer()
	{
		delete [] _data;
	}
	time_t firstRequestTime;
	time_t lastRequestTime;
	int numrequests;
	void Clear()
	{
		memset(_data,0,_memlen);
		firstRequestTime=0;
		lastRequestTime=0;
	}

	virtual void Aggregrate(LogEntry *le)
	{
		// adjust local data

		if (!firstRequestTime) firstRequestTime=le->endTime;
		lastRequestTime=max(lastRequestTime,le->endTime);
		numrequests++;

		// adjust features
		char *dataOffset=_data;
		for(auto i=_features.begin();i!=_features.end();i++)
		{
			auto f=*i;
			f.first->Aggregrate(le,this,dataOffset,_featureData+f.second);
			dataOffset+=f.first->GetDataSize();
		}
	}
};
#endif
