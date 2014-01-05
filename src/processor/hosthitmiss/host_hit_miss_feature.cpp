#include <algorithm>
#include <iostream>
#include <ostream>
#include <string.h>
#include "../log_entry.h"
#include "host_hit_miss_feature.h"

using namespace std;

/* constructor, sliding window of <period> seconds divided in several ranges of <range> seconds
 */
HostHitMissFeature::HostHitMissFeature(int period,int range)
{
	this->_current=0;

	this->_period=period;
	this->_range=range;
	_datalength=2+(this->_range+this->_period-1)/range;
	_pos=0;
	data=new HitMissRange[_datalength];
	memset(data,0,sizeof(HitMissRange)*_datalength);
}
HostHitMissFeature::~HostHitMissFeature()
{
	delete [] data;
}
void HostHitMissFeature::Dump()
{
	cout << this->data[_pos].rangeRatio << "->" << this->data[_pos].periodRatio << endl;
}
/* Process LogEntry and calculate ratios
 */
void HostHitMissFeature::Aggregrate(LogEntry *le)
{
	_current=std::max(le->endTime,_current); // get current time (earlier entries will shift to the latest time

	
	time_t fromStamp=data[_pos].from; // get start time of current block

	if (fromStamp==0) // new block
	{
		memset(data+_pos,0,sizeof(HitMissRange)); // clear
		
		data[_pos].from=le->endTime; // copy current time
		_current=le->endTime;
		data[_pos].countFromIndex=_pos; // first block
		
	}
	else
	if ((fromStamp+_range)<=_current) // shift to a new block
	{
		
		HitMissRange *previous=data+_pos; // save current block
		
		_pos++; // next block
		_pos%=_datalength; // because we are circular
		
		// init
		data[_pos].reported=0;
		data[_pos].rangeHits=0;
		data[_pos].rangeMisses=0;
		data[_pos].rangeRatio=0;
		data[_pos].rangeTotal=0;
		data[_pos].from=_current;

		// copy from previous
		data[_pos].periodTotal=previous->periodTotal;
		data[_pos].periodHits=previous->periodHits;
		data[_pos].periodMisses=previous->periodMisses;
		data[_pos].totalCountFromStart=previous->totalCountFromStart;
		

		int countFromIndex=previous->countFromIndex;
		time_t countfrom=_current-_period;
		while (data[countFromIndex].from<countfrom) // subtract all which are not fully in range
		{
			data[_pos].periodTotal-=data[countFromIndex].rangeTotal;
			data[_pos].periodHits-=data[countFromIndex].rangeHits;
			data[_pos].periodMisses-=data[countFromIndex].rangeMisses;
			countFromIndex=(countFromIndex+1) % _datalength;			
		}
		data[_pos].countFromIndex=countFromIndex; // set for next shift

		
	}
	HitMissRange *currentRange=data+_pos;
	
	//Adjust totals and ratios for period
	currentRange->totalCountFromStart=currentRange->totalCountFromStart+1;
	currentRange->periodTotal=currentRange->periodTotal+1;
	currentRange->periodHits=currentRange->periodHits+(le->cacheLookupStatus==CacheLookupStatus::Hit ? 1 : 0);
	currentRange->periodMisses=currentRange->periodMisses+(le->cacheLookupStatus==CacheLookupStatus::Miss ? 1 : 0);
	currentRange->periodRatio=((double) currentRange->periodHits)/((double) currentRange->periodTotal);

	//Adjust totals and ratios for current range
	currentRange->rangeTotal=currentRange->rangeTotal+1;
	currentRange->rangeHits=currentRange->rangeHits+(le->cacheLookupStatus==CacheLookupStatus::Hit ? 1 : 0);
	currentRange->rangeMisses=currentRange->rangeMisses+(le->cacheLookupStatus==CacheLookupStatus::Miss ? 1 : 0);
	currentRange->rangeRatio=((double) currentRange->rangeHits)/((double) currentRange->rangeTotal);

};
