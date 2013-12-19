#ifndef FEATUREPERCENTAGECONS_H
#define FEATUREPERCENTAGECONS_H
#include "feature.h"


/* calculate the percentage of content (text/html) requests which are consecutive, ie. in the same folder
 * should be moved to hash based path to save space
 */
class FeaturePercentageConsecutiveRequests:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
