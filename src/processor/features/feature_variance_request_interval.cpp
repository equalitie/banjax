#include <string.h>
#include "../log_entry.h"
#include "../utils/std_dev.h"
#include "feature_container.h"
#include "feature_variance_request_interval.h"




int FeatureVarianceRequestInterval::GetDataSize()
{
	return sizeof(StdDev);
}


void FeatureVarianceRequestInterval::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{

	UNUSED(fc);
	StdDev *myData=(StdDev *) data;
	myData->AddCumulative(le->endTime);
	*featureValue=myData->GetStdDevP();
}
