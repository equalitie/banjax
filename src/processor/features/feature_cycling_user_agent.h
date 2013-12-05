#ifndef FEATURECYCLINGUSERAGENT_H
#define FEATURECYCLINGUSERAGENT_H
#include "feature.h"

class FeatureCyclingUserAgent:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
