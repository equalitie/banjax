#ifndef FEATUREHTTPSTATUSRATIO_H
#define FEATUREHTTPSTATUSRATIO_H

#include "feature.h"

/* Calculate the ratio (error status 4xx)/(total number of requests)
 */
class FeatureHTTPStatusRatio:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
