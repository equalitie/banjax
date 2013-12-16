#ifndef FEATURECYCLINGUSERAGENT_H
#define FEATURECYCLINGUSERAGENT_H
#include "feature.h"

#define MAX_USER_AGENTS 11

class FeatureCyclingUserAgent:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
