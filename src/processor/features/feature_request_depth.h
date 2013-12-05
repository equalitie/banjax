#ifndef FEATUREREQUESTDEPTH_H
#define FEATUREREQUESTDEPTH_H
#include "feature.h"

class FeatureRequestDepth:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
