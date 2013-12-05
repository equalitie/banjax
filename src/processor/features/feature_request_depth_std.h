#ifndef FEATUREREQUESTDEPTHSTD_H
#define FEATUREREQUESTDEPTHSTD_H
#include "feature.h"

class FeatureRequestDepthStd:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
