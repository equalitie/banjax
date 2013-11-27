#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeatureSessionLength.h"

void FeatureSessionLength::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,float *featureValue)
{
	UNUSED(le);
	UNUSED(data);
	int time=fc->lastRequestTime-fc->firstRequestTime;
	*featureValue=(float) time;
}
