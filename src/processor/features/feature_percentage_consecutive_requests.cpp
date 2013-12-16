#include <string.h>
#include "../utils/string_utils.h"
#include "../log_entry.h"
#include "feature_container.h"
#include "feature_percentage_consecutive_requests.h"


struct pcrqData
{
	char lastfolder[400]; // should change this to sha1
	int consecutiverequests;
	int totalcontentrequests;
};

int FeaturePercentageConsecutiveRequests::GetDataSize()
{
	return sizeof(pcrqData);
}
void FeaturePercentageConsecutiveRequests::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	UNUSED(fc);
	char curfolder[400];
	pcrqData *myData=(pcrqData *) data;
	if (strcmp(le->contenttype,"text/html")==0)
	{		
		myData->totalcontentrequests++;		
		strlcpy(curfolder,le->url,400);
		char *lastslash=strrchr(curfolder,'/');
		if (lastslash)
			lastslash[1]=0;
		if (strcmp(curfolder,myData->lastfolder)==0)
		{
			myData->consecutiverequests++;
		}
		else
		{
			strlcpy(myData->lastfolder,curfolder,400);
		}

		*featureValue=((double)myData->consecutiverequests)/((double)myData->totalcontentrequests);
	}	
	
	
	
}
