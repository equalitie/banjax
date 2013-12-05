#ifndef FEATUREAVERAGETIMEBETWEENREQUESTS_H
#define FEATUREAVERAGETIMEBETWEENREQUESTS_H
#include "feature.h"

class FeatureAverageTimeBetweenRequests:public Feature
{
public:
	int GetDataSize() {return 0;} 
	void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
