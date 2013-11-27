#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeatureAveragePayloadSize.h"


struct avplsData
{
	int totalpayload;
};

int FeatureAveragePayloadSize::GetDataSize()
{
	return sizeof(avplsData);
}
void FeatureAveragePayloadSize::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,float *featureValue)
{
	avplsData *myData=(avplsData *) data;
	myData->totalpayload+=le->payloadsize;
	
	*featureValue=((float)myData->totalpayload)/((float)fc->numrequests);
}