#include <gtest/gtest.h>
#include "../hosthitmiss/host_hit_miss_aggregator.h"
#include "../log_entry.h"
#include "log_entry_test.h"

class GatherRange:public HostHitMissEventListener
{
public:
	HitMissRange *lastRange;
	string lastHost;
	virtual void OnHostHitMissEvent(char *host,HitMissRange *hmrange)
	{
		lastHost=string(host);
		lastRange=hmrange;
	}
};

/* test periods + transition of sliding window */
TEST(host_hit_miss_aggregator,test_period)
{
	auto listener=new GatherRange();
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	HostHitMissAggregator hhm(60,5);

	hhm.RegisterEventListener(listener);
	le.cacheLookupStatus=CacheLookupStatus::Hit;

	for (int c=0;c<60;c++)
	{
		le.endTime++;
		hhm.Aggregate(&le);
		EXPECT_EQ(1+c,listener->lastRange->totalCountFromStart);
		EXPECT_EQ(1+(c%5),listener->lastRange->rangeTotal);
		EXPECT_EQ(1+(c%5),listener->lastRange->rangeHits);
		EXPECT_EQ(0,listener->lastRange->rangeMisses); // no misses
		EXPECT_EQ(1+c,listener->lastRange->periodTotal);
		EXPECT_EQ(1+c,listener->lastRange->periodHits);
		EXPECT_EQ(0,listener->lastRange->periodMisses); // no misses (yet)

	}
	// we will now cross the period
	// we take one range extra so we always have more samples than the period
	// every 5 seconds we will subtract the last period
	// a better way would be to create a event on closure of a range
	for (int c=0;c<60;c++)
	{
		le.endTime++;
		hhm.Aggregate(&le);
		EXPECT_EQ(61+c,listener->lastRange->totalCountFromStart);
		EXPECT_EQ(1+(c%5),listener->lastRange->rangeTotal);
		EXPECT_EQ(1+(c%5),listener->lastRange->rangeHits);
		EXPECT_EQ(0,listener->lastRange->rangeMisses); // no misses
		EXPECT_EQ(61+c%5,listener->lastRange->periodTotal);
		EXPECT_EQ(61+c%5,listener->lastRange->periodHits); // every 5 seconds we will subtract the last period
		EXPECT_EQ(0,listener->lastRange->periodMisses); // no misses (yet)

	}



}
/* test periods + transition of sliding window with a big gap 60 seconds)*/
TEST(host_hit_miss_aggregator,test_period_with_gap)
{
	auto listener=new GatherRange();
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	HostHitMissAggregator hhm(60,5);

	hhm.RegisterEventListener(listener);
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	hhm.Aggregate(&le);
	EXPECT_EQ(1,listener->lastRange->totalCountFromStart);
	EXPECT_EQ(1,listener->lastRange->rangeTotal);
	EXPECT_EQ(1,listener->lastRange->rangeHits);
	EXPECT_EQ(0,listener->lastRange->rangeMisses); // no misses
	EXPECT_EQ(1,listener->lastRange->periodTotal);
	EXPECT_EQ(1,listener->lastRange->periodHits);
	EXPECT_EQ(0,listener->lastRange->periodMisses); // no misses (yet)

	le.endTime+=60;
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	hhm.Aggregate(&le);
	le.cacheLookupStatus=CacheLookupStatus::Miss;
	hhm.Aggregate(&le);

	// expect a new range, but will still count
	EXPECT_EQ(3,listener->lastRange->totalCountFromStart);
	EXPECT_EQ(2,listener->lastRange->rangeTotal);
	EXPECT_EQ(1,listener->lastRange->rangeHits);
	EXPECT_EQ(1,listener->lastRange->rangeMisses); // no misses
	EXPECT_EQ(3,listener->lastRange->periodTotal);
	EXPECT_EQ(2,listener->lastRange->periodHits);
	EXPECT_EQ(1,listener->lastRange->periodMisses);

	le.endTime+=5;
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	hhm.Aggregate(&le);
	// expect a new range, but will still count
	EXPECT_EQ(4,listener->lastRange->totalCountFromStart);
	EXPECT_EQ(1,listener->lastRange->rangeTotal);
	EXPECT_EQ(1,listener->lastRange->rangeHits);
	EXPECT_EQ(0,listener->lastRange->rangeMisses); // no misses
	EXPECT_EQ(3,listener->lastRange->periodTotal); // first is gone, this is not 4
	EXPECT_EQ(2,listener->lastRange->periodHits); // first is gone, this is not 3
	EXPECT_EQ(1,listener->lastRange->periodMisses);

}

/* test periods + transition of sliding window with a big gap 60 seconds)*/
TEST(host_hit_miss_aggregator,test_different_hosts)
{
	auto listener=new GatherRange();
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	HostHitMissAggregator hhm(60,5);

	hhm.RegisterEventListener(listener);
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	strcpy(le.hostname,"host1");
	hhm.Aggregate(&le);
	EXPECT_EQ(1,listener->lastRange->totalCountFromStart);
	EXPECT_EQ(1,listener->lastRange->rangeTotal);
	EXPECT_EQ(1,listener->lastRange->rangeHits);
	EXPECT_EQ(0,listener->lastRange->rangeMisses); // no misses
	EXPECT_EQ(1,listener->lastRange->periodTotal);
	EXPECT_EQ(1,listener->lastRange->periodHits);
	EXPECT_EQ(0,listener->lastRange->periodMisses); // no misses (yet)

	le.cacheLookupStatus=CacheLookupStatus::Miss;
	strcpy(le.hostname,"host2");
	hhm.Aggregate(&le);
	EXPECT_EQ(1,listener->lastRange->totalCountFromStart);
	EXPECT_EQ(1,listener->lastRange->rangeTotal);
	EXPECT_EQ(0,listener->lastRange->rangeHits);
	EXPECT_EQ(1,listener->lastRange->rangeMisses); // no misses
	EXPECT_EQ(1,listener->lastRange->periodTotal);
	EXPECT_EQ(0,listener->lastRange->periodHits);
	EXPECT_EQ(1,listener->lastRange->periodMisses); // no misses (yet)

	le.endTime+=5;
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	strcpy(le.hostname,"host1");
	hhm.Aggregate(&le);
	EXPECT_EQ(2,listener->lastRange->totalCountFromStart);
	EXPECT_EQ(1,listener->lastRange->rangeTotal);
	EXPECT_EQ(1,listener->lastRange->rangeHits);
	EXPECT_EQ(0,listener->lastRange->rangeMisses); // no misses
	EXPECT_EQ(2,listener->lastRange->periodTotal);
	EXPECT_EQ(2,listener->lastRange->periodHits);
	EXPECT_EQ(0,listener->lastRange->periodMisses); // no misses (yet)

	hhm.Aggregate(&le);
	EXPECT_EQ(3,listener->lastRange->totalCountFromStart);
	EXPECT_EQ(2,listener->lastRange->rangeTotal);
	EXPECT_EQ(2,listener->lastRange->rangeHits);
	EXPECT_EQ(0,listener->lastRange->rangeMisses); // no misses
	EXPECT_EQ(3,listener->lastRange->periodTotal);
	EXPECT_EQ(3,listener->lastRange->periodHits);
	EXPECT_EQ(0,listener->lastRange->periodMisses); // no misses (yet)

	le.cacheLookupStatus=CacheLookupStatus::Miss;
	strcpy(le.hostname,"host2");
	hhm.Aggregate(&le);
	EXPECT_EQ(2,listener->lastRange->totalCountFromStart);
	EXPECT_EQ(1,listener->lastRange->rangeTotal);
	EXPECT_EQ(0,listener->lastRange->rangeHits);
	EXPECT_EQ(1,listener->lastRange->rangeMisses); // no misses
	EXPECT_EQ(2,listener->lastRange->periodTotal);
	EXPECT_EQ(0,listener->lastRange->periodHits);
	EXPECT_EQ(2,listener->lastRange->periodMisses); // no misses (yet)

}

/* after the previous tests,  we know the timing works so let's generate some random hit/misses
 * and test the ratios
 */
TEST(host_hit_miss_aggregator,test_ratio)
{
	auto listener=new GatherRange();
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	HostHitMissAggregator hhm(60,5);

	hhm.RegisterEventListener(listener);

	for (int c=0;c<4000;c++)
	{

		if (rand()%2)
		{
			le.cacheLookupStatus=CacheLookupStatus::Hit;
		}
		else
		{
			le.cacheLookupStatus=CacheLookupStatus::Miss;
		}
		le.endTime++;
		hhm.Aggregate(&le);
		EXPECT_EQ(listener->lastRange->rangeRatio,
				((double )listener->lastRange->rangeHits)/
				((double )listener->lastRange->rangeTotal)
		);
		EXPECT_EQ(listener->lastRange->periodRatio,
						((double )listener->lastRange->periodHits)/
						((double )listener->lastRange->periodTotal)
				);
	}
}
