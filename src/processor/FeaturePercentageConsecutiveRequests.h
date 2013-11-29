#ifndef FEATUREPERCENTAGECONS_H
#define FEATUREPERCENTAGECONS_H
#include "Feature.h"

class FeaturePercentageConsecutiveRequests:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,float *featureValue);
};
#endif
