#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeatureAverageTimeBetweenRequests.h"



void FeatureAverageTimeBetweenRequests::Aggregrate(LogEntry *le, FeatureContainer *fc,void *data,float *featureValue)
{
	UNUSED(le);
	UNUSED(data);
	int time=fc->lastRequestTime-fc->firstRequestTime;
	if (fc->numrequests==1)
	{
		*featureValue=MAX_IDEAL_SESSION_LENGTH;
	}
	else
	*featureValue=((float) time)/((float) (fc->numrequests-1.0f));
}
