#include <string.h>
#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeatureVarianceRequestInterval.h"
#include "FeatureStdDevData.h"




int FeatureVarianceRequestInterval::GetDataSize()
{
	return sizeof(FeatureStdDevData);
}


void FeatureVarianceRequestInterval::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,float *featureValue)
{

	UNUSED(fc);
	FeatureStdDevData *myData=(FeatureStdDevData *) data;
	myData->AddCumulative(le->endTime);
	*featureValue=myData->GetStdDevP();
}
