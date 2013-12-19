#ifndef FEATUREAVERAGETIMEBETWEENREQUESTS_H
#define FEATUREAVERAGETIMEBETWEENREQUESTS_H
#include "feature.h"


#define MAX_IDEAL_SESSION_LENGTH 1800

/* Calculates average time between requests */
class FeatureAverageTimeBetweenRequests:public Feature
{
public:
	int GetDataSize() {return 0;} // does not need bookkeeping
	void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
