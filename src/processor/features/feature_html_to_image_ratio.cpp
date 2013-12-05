#include <string.h>
#include "../log_entry.h"
#include "feature_container.h"
#include "feature_html_to_image_ratio.h"


struct hirData
{
	int totalimagerequests;
	int totalcontentrequests;
};

int FeatureHtmlToImageRatio::GetDataSize()
{
	return sizeof(hirData);
}
void FeatureHtmlToImageRatio::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	UNUSED(fc);

	hirData *myData=(hirData *) data;
	if (strcmp(le->contenttype,"text/html")==0)
	{		
		myData->totalcontentrequests++;		
		*featureValue=((double)myData->totalimagerequests)/((double)myData->totalcontentrequests);
	}	
	else
		if (memcmp(le->contenttype,"image/",6)==0)
		{
			myData->totalimagerequests++;
			if (myData->totalcontentrequests!=0)
				*featureValue=((double)myData->totalimagerequests)/((double)myData->totalcontentrequests);
		}
	
	
}
