#ifndef FEATUREAVERAGETIMEBETWEENREQUESTS_H
#define FEATUREAVERAGETIMEBETWEENREQUESTS_H
#include "Feature.h"

class FeatureAverageTimeBetweenRequests:public Feature
{
public:
	int GetDataSize() {return 0;} 
	void Aggregrate(LogEntry *le,FeatureContainer *,void *data,float *featureValue);
};
#endif
