#include <string.h>
#include "../utils/hashfunctions/crc32.h"
#include "../log_entry.h"
#include "feature_container.h"
#include "feature_cycling_user_agent.h"

/* useragent hash entry + count */
struct cuaEntry 
{
	unsigned int hash;
	int counter;
};

/* a collection of useragent entries
 * the first entry is an overflow entry
 * This is an array, this is not a problem performancewise as
 * the array should be small
 */
struct cuaData
{
	struct cuaEntry userAgents[MAX_USER_AGENTS];
	int numAgents;
	int maxRequests;
};

/* add the useragent and calculate the ratio (largest number of requests of a single useragent)/(total number of requests) */
void FeatureCyclingUserAgent::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
{
	cuaData *myData=(cuaData *) data;

	unsigned int crc32=Crc32_ComputeBuf(0,le->userAgent,strlen(le->userAgent));
	unsigned int *hashValue=&crc32;
	int curVal=0;
	int i;
	/* find the useragent by it's CRC32 hashvalue
	 * skip the first entry, this is overflow
	 */
	for(i=1;i<=myData->numAgents;i++)
	{
		if(myData->userAgents[i].hash==*hashValue)
		{
			curVal=++(myData->userAgents[i].counter);
			break;
		}
	}
	if (i>myData->numAgents) // not found?
	{
		if (i==MAX_USER_AGENTS ) // if there is no space, add to overflow
		{
			// do not count the overflow as max, we could skip this
			++(myData->userAgents[0].counter);

		}
		else
		{
			//create a new entry
			myData->numAgents++;
			myData->userAgents[myData->numAgents].hash=*hashValue;
			curVal=myData->userAgents[myData->numAgents].counter=1;
		}

	}
	// get largest useragent and calculate ratio
	myData->maxRequests=max(curVal,myData->maxRequests);
	*featureValue=((double) myData->maxRequests)/((double)fc->numRequests);
	
	
}


int FeatureCyclingUserAgent::GetDataSize()
{
	return sizeof(cuaData);
}
