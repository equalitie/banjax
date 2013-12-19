#include <string.h>
#include "../log_entry.h"
#include "../utils/std_dev.h"
#include "feature_container.h"
#include "feature_request_depth_std.h"



// local data is StdDev struct
int FeatureRequestDepthStd::GetDataSize()
{
	return sizeof(StdDev);
}

/* calulate the stddev of the content request depths, -1 if there is no data yet
 */
void FeatureRequestDepthStd::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	UNUSED(fc);
	StdDev *myData=(StdDev *) data;
	if (strcmp(le->contenttype,"text/html")==0)
	{
		int depth=0;
		char *s=le->url;
		// count slashes in the string
		while (*s) {if (*s=='/') depth++;s++;}
		/* this is not present in the python version, might need this */
		//if (depth==0) depth=1; // no depth is depth 1 ''=='/'

		// add data to running stddev
		myData->AddNum((double)depth);
		double value=myData->GetStdDevP();
		// don't know if this is right
		*featureValue=value;// ? value : -1;
	}
	else // if we have seen no content the feature is -1
		if (fc->numRequests==1) *featureValue=-1;
}
