#ifndef FEATUREAVERAGEPAYLOADSIZE_H
#define FEATUREAVERAGEPAYLOADSIZE_H
#include "feature.h"

class FeatureAveragePayloadSize:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
