#include <string.h>
#include "../log_entry.h"
#include "feature_container.h"
#include "feature_html_to_image_ratio.h"

/* local data */
struct hirData
{
	int totalimagerequests;
	int totalcontentrequests;
};

/* calculate the ratio (image requests)/(html requests)
 */
void FeatureHtmlToImageRatio::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	UNUSED(fc);

	hirData *myData=(hirData *) data;
	if (strcmp(le->contenttype,"text/html")==0) // html page
	{		
		myData->totalcontentrequests++;		
		*featureValue=((double)myData->totalimagerequests)/((double)myData->totalcontentrequests);
	}	
	else
		if (memcmp(le->contenttype,"image/",6)==0) // image
		{
			myData->totalimagerequests++;
			if (myData->totalcontentrequests!=0) // do not adjust ratio if we have no content requests (prevent divide by zero)
				*featureValue=((double)myData->totalimagerequests)/((double)myData->totalcontentrequests);
		}
	
	
}


int FeatureHtmlToImageRatio::GetDataSize()
{
	return sizeof(hirData);
}
