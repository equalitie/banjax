#ifndef FEATUREREQUESTDEPTHSTD_H
#define FEATUREREQUESTDEPTHSTD_H
#include "feature.h"


/* calulate the stddev of the content request depths, -1 if there is no data yet
 */
class FeatureRequestDepthStd:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
