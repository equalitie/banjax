#ifndef FEATURESESSIONLENGTH_H
#define FEATURESESSIONLENGTH_H
#include "feature.h"

/* calculates the length of the session for the LogEntry's which are aggregated */
class FeatureSessionLength:public Feature
{
public:
	virtual int GetDataSize() {return 0;} 
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
