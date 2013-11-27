#include <algorithm>
#include <iostream>
#include <ostream>
#include <string.h>
#include "LogEntry.h"
#include "HostHitMissFeature.h"

using namespace std;


HostHitMissFeature::HostHitMissFeature(int totalRange,int periodRange)
{
	this->current=0;

	this->totalRange=totalRange;
	this->periodRange=periodRange;
	datalength=2+(this->periodRange+this->totalRange-1)/periodRange;
	pos=0;
	data=new HitMissRange[datalength];
	memset(data,0,sizeof(HitMissRange)*datalength);
}
HostHitMissFeature::~HostHitMissFeature()
{
	delete [] data;
}
void HostHitMissFeature::Dump()
{
	cout << this->data[pos].rangeRatio << "->" << this->data[pos].ratio << endl;
}

void HostHitMissFeature::Aggregrate(LogEntry *le)
{
	current=std::max(le->endTime,current);

	
	time_t fromStamp=data[pos].from;

	if (fromStamp==0) // new block
	{
		memset(data+pos,0,sizeof(HitMissRange));		
		
		data[pos].from=le->endTime;
		current=le->endTime;
		data[pos].countFromIndex=pos; // same
		
	}
	else
	if (fromStamp+periodRange<current)
	{
		
		HitMissRange *previous=data+pos;
		
		pos++;
		pos%=datalength;	
		
		data[pos].reported=0;
		data[pos].rangeHits=0;
		data[pos].rangeMisses=0;
		data[pos].rangeRatio=0;
		data[pos].rangeTotal=0;
		data[pos].from=current;

		data[pos].total=previous->total;
		data[pos].hits=previous->hits;
		data[pos].misses=previous->misses;
		data[pos].totalCount=previous->totalCount;
		
		int countFromIndex=previous->countFromIndex;
		time_t countfrom=current-totalRange;
		while (data[countFromIndex].from<countfrom) // subtract all which are not fully in range
		{
			data[pos].total-=data[countFromIndex].rangeTotal;
			data[pos].hits-=data[countFromIndex].rangeHits;
			data[pos].misses-=data[countFromIndex].rangeMisses;			
			countFromIndex=(countFromIndex+1) % datalength;			
		}
		data[pos].countFromIndex=countFromIndex;

		
	}
	HitMissRange *currentRange=data+pos;
	
	currentRange->totalCount=currentRange->totalCount+1;
	currentRange->total=currentRange->total+1;	
	currentRange->hits=currentRange->hits+(le->cacheLookupStatus==CacheLookupStatus::Hit ? 1 : 0);
	currentRange->misses=currentRange->misses+(le->cacheLookupStatus==CacheLookupStatus::Miss ? 1 : 0);
	currentRange->ratio=((float) currentRange->hits)/((float) currentRange->total);

	currentRange->rangeTotal=currentRange->rangeTotal+1;
	currentRange->rangeHits=currentRange->rangeHits+(le->cacheLookupStatus==CacheLookupStatus::Hit ? 1 : 0);
	currentRange->rangeMisses=currentRange->rangeMisses+(le->cacheLookupStatus==CacheLookupStatus::Miss ? 1 : 0);
	currentRange->rangeRatio=((float) currentRange->rangeHits)/((float) currentRange->rangeTotal);

};
