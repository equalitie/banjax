#include <string.h>
#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeatureRequestDepthStd.h"
#include "FeatureStdDevData.h"




int FeatureRequestDepthStd::GetDataSize()
{
	return sizeof(FeatureStdDevData);
}


void FeatureRequestDepthStd::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,float *featureValue)
{
	UNUSED(fc);
	FeatureStdDevData *myData=(FeatureStdDevData *) data;
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
