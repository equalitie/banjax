#ifndef FEATUREREQUESTDEPTHSTD_H
#define FEATUREREQUESTDEPTHSTD_H
#include "Feature.h"

class FeatureRequestDepthStd:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,float *featureValue);
};
#endif
