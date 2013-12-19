#include "../log_entry.h"
#include "feature_container.h"
#include "feature_average_payload_size.h"


/* local data structure */
struct avplsData
{
	int totalpayload; // total payload in bytes for all requests
};

/* add payload size to and calculate average */
void FeatureAveragePayloadSize::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	avplsData *myData=(avplsData *) data;
	myData->totalpayload+=le->payloadsize;
	
	*featureValue=((double)myData->totalpayload)/((double)fc->numRequests);
}

int FeatureAveragePayloadSize::GetDataSize()
{
	return sizeof(avplsData);
}
