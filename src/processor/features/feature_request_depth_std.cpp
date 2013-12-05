#include <string.h>
#include "../log_entry.h"
#include "../utils/std_dev.h"
#include "feature_container.h"
#include "feature_request_depth_std.h"




int FeatureRequestDepthStd::GetDataSize()
{
	return sizeof(StdDev);
}


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
		//if (depth==0) depth=1; // no depth is depth 1 ''=='/'
		myData->AddNum((double)depth);
		double value=myData->GetStdDevP();
		// don't know if this is right
		*featureValue=value ? value : -1;
	}
	else
		if (fc->numrequests==1) *featureValue=-1;
}
