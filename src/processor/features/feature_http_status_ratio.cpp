#include "../log_entry.h"
#include "feature_container.h"
#include "feature_http_status_ratio.h"

/* local data */
struct fhsrData
{
	int error_requests;
};



/* Calculate the ratio (error status 4xx)/(total number of requests)
 */
void FeatureHTTPStatusRatio::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	fhsrData *myData=(fhsrData *) data;
	if (le->httpCode>=400 && le->httpCode<500)
		myData->error_requests++;
	*featureValue=((double)myData->error_requests)/((double) fc->numRequests);
}


int FeatureHTTPStatusRatio::GetDataSize()
{
	return sizeof(fhsrData);
}
