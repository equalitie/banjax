#include "../log_entry.h"
#include "feature_container.h"
#include "feature_session_length.h"

/* calculates the length of the session for the aggregrated log entries */
void FeatureSessionLength::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	UNUSED(le);
	UNUSED(data);
	int time=fc->lastRequestTime-fc->firstRequestTime; // firstRequestTime and lastRequestTime from the featureContainer
	*featureValue=(double) time;
}
