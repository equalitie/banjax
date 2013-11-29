#ifndef FEATURECYCLINGUSERAGENT_H
#define FEATURECYCLINGUSERAGENT_H
#include "Feature.h"

class FeatureCyclingUserAgent:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,float *featureValue);
};
#endif
