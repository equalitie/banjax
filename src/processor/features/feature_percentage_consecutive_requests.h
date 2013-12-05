#ifndef FEATUREPERCENTAGECONS_H
#define FEATUREPERCENTAGECONS_H
#include "feature.h"

class FeaturePercentageConsecutiveRequests:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
