#include <string.h>
#include "utils/stringutils.h"
#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeaturePercentageConsecutiveRequests.h"


struct pcrqData
{
	char lastfolder[400];
	int consecutiverequests;
	int totalcontentrequests;
};

int FeaturePercentageConsecutiveRequests::GetDataSize()
{
	return sizeof(pcrqData);
}
void FeaturePercentageConsecutiveRequests::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,float *featureValue)
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

		*featureValue=((float)myData->consecutiverequests)/((float)myData->totalcontentrequests);
	}	
	
	
	
}
