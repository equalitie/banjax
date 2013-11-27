#include <string.h>
#include "hashfunctions/crc32.h"
#include "LogEntry.h"
#include "FeatureContainer.h"
#include "FeatureCyclingUserAgent.h"

#define MAX_AGENTS 11
struct cuaEntry 
{
	unsigned int hash;
	int counter;
};
struct cuaData
{
	struct cuaEntry userAgents[MAX_AGENTS];
	int numAgents;
	int maxRequests;
};

int FeatureCyclingUserAgent::GetDataSize()
{
	return sizeof(cuaData);
}
void FeatureCyclingUserAgent::Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,float *featureValue)
{
	cuaData *myData=(cuaData *) data;

	//sha1(le->userAgent);
	//sha1::calc(le->userAgent,strlen(le->userAgent),hash);
	unsigned int crc32=Crc32_ComputeBuf(0,le->userAgent,strlen(le->userAgent));
	unsigned int *hashValue=&crc32;
	int curVal=0;
	int i;
	for(i=1;i<=myData->numAgents;i++) //0 = other
	{
		if(myData->userAgents[i].hash==*hashValue)
		{
			curVal=++(myData->userAgents[i].counter);
			break;
		}
	}
	if (i>myData->numAgents)
	{
		if (i==MAX_AGENTS)
		{
			// do not count the overflow as max
			++(myData->userAgents[0].counter);

		}
		else
		{
			myData->numAgents++;
			myData->userAgents[myData->numAgents].hash=*hashValue;
			curVal=myData->userAgents[myData->numAgents].counter=1;
			
		}

	}
	myData->maxRequests=max(curVal,myData->maxRequests);
	*featureValue=((float) myData->maxRequests)/((float)fc->numrequests);
	
	
}
