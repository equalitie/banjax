#ifndef FEATURECYCLINGUSERAGENT_H
#define FEATURECYCLINGUSERAGENT_H
#include "feature.h"

#define MAX_USER_AGENTS 11 // the maximum number of useragents we store (this is including the overflow)

/* add the useragent and calculate the ratio (largest number of requests of a single useragent)/(total number of requests) */
class FeatureCyclingUserAgent:public Feature
{
public:
	virtual int GetDataSize();

	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
