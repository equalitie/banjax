#include "../log_entry.h"
#include "feature_container.h"
#include "feature_session_length.h"

void FeatureSessionLength::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	UNUSED(le);
	UNUSED(data);
	int time=fc->lastRequestTime-fc->firstRequestTime;
	*featureValue=(double) time;
}
