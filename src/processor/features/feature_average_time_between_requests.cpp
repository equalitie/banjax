#include "../log_entry.h"
#include "feature_container.h"
#include "feature_average_time_between_requests.h"


/* Calculates average time between requests */
void FeatureAverageTimeBetweenRequests::Aggregrate(LogEntry *le, FeatureContainer *fc,void *data,double *featureValue)
{
	UNUSED(le);
	UNUSED(data);
	int time=fc->lastRequestTime-fc->firstRequestTime;
	if (fc->numRequests==1)
	{
		*featureValue=MAX_IDEAL_SESSION_LENGTH;
	}
	else
	*featureValue=((double) time)/((double) (fc->numRequests-1.0));
}
