#include "../log_entry.h"
#include "feature_container.h"
#include "feature_average_payload_size.h"


struct avplsData
{
	int totalpayload;
};

int FeatureAveragePayloadSize::GetDataSize()
{
	return sizeof(avplsData);
}
void FeatureAveragePayloadSize::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	avplsData *myData=(avplsData *) data;
	myData->totalpayload+=le->payloadsize;
	
	*featureValue=((double)myData->totalpayload)/((double)fc->numrequests);
}
