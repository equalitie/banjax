#include <string.h>
#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeatureHtmlToImageRatio.h"


struct hirData
{
	int totalimagerequests;
	int totalcontentrequests;
};

int FeatureHtmlToImageRatio::GetDataSize()
{
	return sizeof(hirData);
}
void FeatureHtmlToImageRatio::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,float *featureValue)
{
	UNUSED(fc);

	hirData *myData=(hirData *) data;
	if (strcmp(le->contenttype,"text/html")==0)
	{		
		myData->totalcontentrequests++;		
		*featureValue=((float)myData->totalimagerequests)/((float)myData->totalcontentrequests);
	}	
	else
		if (memcmp(le->contenttype,"image/",6)==0)
		{
			myData->totalimagerequests++;
			if (myData->totalcontentrequests!=0)
				*featureValue=((float)myData->totalimagerequests)/((float)myData->totalcontentrequests);			
		}
	
	
}
