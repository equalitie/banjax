#include <string.h>
#include "../log_entry.h"
#include "feature_container.h"
#include "feature_request_depth.h"

/* local data */
struct rqdData
{
	int totaldepth;
	int totalcontentrequests;
};



/* calculates the average request depth for content (text/html) pages, 0 if no content pages have been seen  */
void FeatureRequestDepth::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	UNUSED(fc);
	rqdData *myData=(rqdData *) data;
	if (strcmp(le->contenttype,"text/html")==0)
	{
		int depth=0;
		char *s=le->url;
		// count slashes in the string
		while (*s) {if (*s=='/') depth++;s++;}
		if (depth==0) depth=1; // no depth is depth 1 ''=='/'
		myData->totalcontentrequests++;
		myData->totaldepth+=depth;
		*featureValue=((double)myData->totaldepth)/((double)myData->totalcontentrequests);
	}	
}

int FeatureRequestDepth::GetDataSize()
{
	return sizeof(rqdData);
}
