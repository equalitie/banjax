#ifndef HOSTHITMISSFEATURE_H
#define HOSTHITMISSFEATURE_H
#include <list>
struct LogEntry;

/* a range entry in the period */
struct HitMissRange
{	
	int reported; // if true, the previous record will be reported
	time_t from; // start time

	int rangeTotal; // total entries for this range
	int rangeHits; // hits in this range
	int rangeMisses; // misses in this range
	double rangeRatio; // hit/total in this range

	
	int periodTotal; // total entries for the period to which this range belongs
	int periodHits; // hits for the period to which this range belongs
	int periodMisses; // misses for the period
	double periodRatio; // hit/total in this period

	
	int countFromIndex; // the totals are counted from this index in the HostHitMissFeature ringbuffer
	int totalCountFromStart;

};

/* This class runs the sliding window with hit/miss/total/ratio data
 * over a <period> with several ranges of <range> long,
 * it does this by using a circular buffer
 */
class HostHitMissFeature
{
	time_t _current; // _current time
	int _pos; // position in circular buffer
	int _datalength; // size of the circular buffer
	int _range; // range within a period
	int _period;	 // period on which to test, note: we do overflow one _range
	HitMissRange *data; // circular buffer
public:
	/* constructor, sliding window of <period> seconds divided in several ranges of <range> seconds
	 */
	HostHitMissFeature(int period,int range);
	/* Process LogEntry and calculate ratios
	 */
	void Aggregrate(LogEntry *le);
	/* Get current range entry
	 */
	HitMissRange *CurrentHitMissRange() {return data+_pos;}
	/* destructor
	 */
	~HostHitMissFeature();
	void Dump();
};
#endif
