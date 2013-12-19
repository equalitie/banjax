#ifndef FEATUREREQUESTDEPTH_H
#define FEATUREREQUESTDEPTH_H
#include "feature.h"

/* calculates the average request depth for content (text/html) pages, 0 if no content pages have been seen */
class FeatureRequestDepth:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
