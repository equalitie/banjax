#ifndef FEATUREHTTPSTATUSRATIO_H
#define FEATUREHTTPSTATUSRATIO_H

#include "feature.h"

class FeatureHTTPStatusRatio:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
