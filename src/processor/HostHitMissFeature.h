#ifndef HOSTHITMISSFEATURE_H
#define HOSTHITMISSFEATURE_H
#include <list>
struct LogEntry;
struct HitMissRange
{	
	int reported;
	time_t from;
	int rangeInSeconds;
	int rangeTotal;
	int rangeHits;
	int rangeMisses;
	float rangeRatio;

	
	int total;
	int hits;
	int misses;	
	float ratio;

	
	int countFromIndex;
	int totalCount;

};
class HostHitMissFeature
{
	time_t current;
	int pos;
	int datalength;
	int periodRange;
	int totalRange;	
	HitMissRange *data;
public:
	HostHitMissFeature(int totalRange,int periodRange);
	void Aggregrate(LogEntry *le);
	HitMissRange *CurrentHitMissRage() {return data+pos;}
	~HostHitMissFeature();
	void Dump();
};
#endif