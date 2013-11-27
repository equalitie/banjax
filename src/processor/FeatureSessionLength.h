#include "Feature.h"

class FeatureSessionLength:public Feature
{
public:
	virtual int GetDataSize() {return 0;} 
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,float *featureValue);
};