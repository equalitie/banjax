#ifndef FEATUREAVERAGEPAYLOADSIZE_H
#define FEATUREAVERAGEPAYLOADSIZE_H
#include "feature.h"

/* calculate the average payload size for LogEntrys
 */
class FeatureAveragePayloadSize:public Feature
{
public:

	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);

};
#endif
