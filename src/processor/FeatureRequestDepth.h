#ifndef FEATUREREQUESTDEPTH_H
#define FEATUREREQUESTDEPTH_H
#include "Feature.h"

class FeatureRequestDepth:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,float *featureValue);
};
#endif
