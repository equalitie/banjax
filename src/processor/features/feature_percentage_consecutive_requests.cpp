#include <string.h>
#include "../utils/string_utils.h"
#include "../log_entry.h"
#include "feature_container.h"
#include "feature_percentage_consecutive_requests.h"

/* local data */
struct pcrqData
{
	char lastfolder[400]; // last folder seen, should change this to sha1
	int consecutiverequests;
	int totalcontentrequests;
};

/* calculate the percentage of content (text/html) requests which are consecutive, ie. in the same folder
 * should be moved to hash based path to save space
 */
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

int FeaturePercentageConsecutiveRequests::GetDataSize()
{
	return sizeof(pcrqData);
}
