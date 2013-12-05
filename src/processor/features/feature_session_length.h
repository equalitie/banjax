#ifndef FEATURESESSIONLENGTH_H
#define FEATURESESSIONLENGTH_H
#include "feature.h"

class FeatureSessionLength:public Feature
{
public:
	virtual int GetDataSize() {return 0;} 
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
