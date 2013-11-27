#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeatureHTTPStatusRatio.h"


struct fhsrData
{
	int error_requests;
};

int FeatureHTTPStatusRatio::GetDataSize()
{
	return sizeof(fhsrData);
}
void FeatureHTTPStatusRatio::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,float *featureValue)
{
	fhsrData *myData=(fhsrData *) data;
	if (le->httpCode>=400 && le->httpCode<500)
		myData->error_requests++;
	*featureValue=((float)myData->error_requests)/((float) fc->numrequests);
}