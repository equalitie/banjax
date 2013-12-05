#include "../log_entry.h"
#include "feature_container.h"
#include "feature_http_status_ratio.h"


struct fhsrData
{
	int error_requests;
};

int FeatureHTTPStatusRatio::GetDataSize()
{
	return sizeof(fhsrData);
}
void FeatureHTTPStatusRatio::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	fhsrData *myData=(fhsrData *) data;
	if (le->httpCode>=400 && le->httpCode<500)
		myData->error_requests++;
	*featureValue=((double)myData->error_requests)/((double) fc->numrequests);
}
