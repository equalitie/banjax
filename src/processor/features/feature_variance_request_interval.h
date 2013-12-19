#ifndef FEATUREVARIANCEREQUESTINTERVAL_H
#define FEATUREVARIANCEREQUESTINTERVAL_H
#include "feature.h"


/* calculate the stddev of the request intervals */
class FeatureVarianceRequestInterval:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
