#include <gtest/gtest.h>
#include "../botbanger/bot_banger_aggregator.h"
#include "log_entry_test.h"
#include "feature_set_value.h"


class EvictListener:public BotBangerEventListener
{
	vector<string> &_evictList;
public:
	EvictListener(vector<string> &evictList):
		BotBangerEventListener(),
		_evictList(evictList)
	{;}
	virtual void OnFeatureEvent(char *key,double *features,int numFeatures)
	{
		UNUSED(key);
		UNUSED(features);
		UNUSED(numFeatures);
	}
	virtual void OnEvictEvent(string key) {
		_evictList.push_back(key);
	} // not mandatory to implement
};

class FeatureEventListener:public BotBangerEventListener
{
public:
	double *features;
	int numFeatures;
	string key;
	FeatureEventListener():BotBangerEventListener(),features(NULL)
	{

	}
	virtual void OnFeatureEvent(char *key,double *features,int numFeatures)
	{
		this->features=features;
		this->numFeatures=numFeatures;
		this->key=string(key);

	}
};

// test eviction on a single timestamp
TEST(bot_banger_aggregator,eviction_test_same_timestamp)
{
	vector<string> evictedIps;
	BotBangerAggregator bag(100,1800);
	bag.RegisterEventListener(new EvictListener(evictedIps));
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	for (int c=0;c<100;c++)
	{
		sprintf(le.useraddress,"50.50.50.%d",c);
		bag.Aggregate(&le);
	}
	// we now have 100 ip's
	EXPECT_EQ(0,evictedIps.size());

	// add 101st , trigger eviction
	sprintf(le.useraddress,"50.50.50.%d",100);
	bag.Aggregate(&le);

	// there should be 10 unique ips which are evicted, doesn't matter which, because the
	// timestamp is the same for all entries but, last one should not be evicted
	EXPECT_EQ(10,evictedIps.size());
	set<string> checkUnique;
	for(auto i=evictedIps.begin();i!=evictedIps.end();i++)
	{
		EXPECT_STRCASENE("50.50.50.100",(*i).c_str()); // last one should not be in list
		checkUnique.insert(*i);

	}
	EXPECT_EQ(10,checkUnique.size()); // there should be 10 unique entries in the evict list

	evictedIps.clear();
	checkUnique.clear();

	// let's add 9 more, fill it up till 100
	for (int c=101;c<110;c++)
	{
		sprintf(le.useraddress,"50.50.50.%d",c);
		bag.Aggregate(&le);
	}
	// should not be any evicted IP's
	EXPECT_EQ(0,evictedIps.size());

	// add 111th, should trigger eviction
	sprintf(le.useraddress,"50.50.50.%d",110);
	bag.Aggregate(&le);

	EXPECT_EQ(10,evictedIps.size());
	for(auto i=evictedIps.begin();i!=evictedIps.end();i++)
	{
		EXPECT_STRCASENE("50.50.50.100",(*i).c_str()); // last one should not be in list
		checkUnique.insert(*i);

	}
	EXPECT_EQ(10,checkUnique.size()); // there should be 10 unique entries in the evict list
}

// test eviction with differing timestamp
TEST(bot_banger_aggregator,eviction_test_ascending_timestamp)
{
	vector<string> evictedIps;
	BotBangerAggregator bag(100,1800);
	bag.RegisterEventListener(new EvictListener(evictedIps));
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	// fill with
	for (int c=0;c<100;c++)
	{
		le.endTime++;
		sprintf(le.useraddress,"50.50.50.%d",c);
		bag.Aggregate(&le);
	}
	// we now have 100 ip's
	EXPECT_EQ(0,evictedIps.size());

	// add 101st , trigger eviction
	sprintf(le.useraddress,"50.50.50.%d",100);
	le.endTime++;
	bag.Aggregate(&le);

	// there should be 10 unique ips which are evicted, these should be
	// the first ones, with the oldest timestamp
	//, last one should not be evicted
	EXPECT_EQ(10,evictedIps.size());

	int n=0;
	char temp[100];

	for(auto i=evictedIps.begin();i!=evictedIps.end();i++,n++)
	{

		sprintf(temp,"50.50.50.%d",n);
		EXPECT_STREQ(temp,(*i).c_str());
	}


	evictedIps.clear();

	// let's add 9 more, fill it up till 100
	for (int c=101;c<110;c++)
	{
		le.endTime++;
		sprintf(le.useraddress,"50.50.50.%d",c);
		bag.Aggregate(&le);
	}

	// let's add 90 entries from 10 till 99 with fresh timestamps, this should ensure
	// 100 till 109 will be flushed, when a new entry is added

	for (int c=10;c<100;c++)
	{
		le.endTime++;
		sprintf(le.useraddress,"50.50.50.%d",c);
		bag.Aggregate(&le);
	}

	EXPECT_EQ(0,evictedIps.size());

	// add 111th, should trigger eviction
	le.endTime++;
	sprintf(le.useraddress,"50.50.50.%d",110);
	bag.Aggregate(&le);

	EXPECT_EQ(10,evictedIps.size());
	n=100; // we now expect 100 till 109 to be flushed
	for(auto i=evictedIps.begin();i!=evictedIps.end();i++,n++)
	{
		sprintf(temp,"50.50.50.%d",n);
		EXPECT_STREQ(temp,(*i).c_str());

	}
}


/* heSet the feature to number of requests */
class FeatureAddOne:public Feature
{
public:

	virtual int GetDataSize() {return 0;}
	/* Set the feature to number of requests
	 */
	virtual void Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
	{
		UNUSED(le);
		UNUSED(fc);
		UNUSED(data);
		*featureValue=*featureValue+1;
	}
};

class FeatureGetFeatureContainer:public Feature
{
public:

	FeatureContainer *lastContainer;
	FeatureGetFeatureContainer():Feature(),lastContainer(NULL)
	{
	}
	virtual int GetDataSize() {return 0;}
	/* Set the feature to number of requests
	 */
	virtual void Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
	{
		UNUSED(le);
		UNUSED(featureValue);
		UNUSED(data);
		lastContainer=fc;
	}
};



TEST(bot_banger_aggregator,timeout_test)
{
	BotBangerAggregator bag(100,1800);
	auto gfc=new FeatureGetFeatureContainer();
	bag.RegisterFeature(new FeatureAddOne(),0);
	bag.RegisterFeature(gfc,1);
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	time_t firstReqTime=le.endTime;
	time_t lastReqTime=0;
	for (int c=0;c<2000;c++)
	{
		int nr;
		le.endTime+=c;
		lastReqTime=le.endTime;
		bag.Aggregate(&le);
		nr=c+1;
		if (c>1800)
		{
			nr=1; // if items are 1800 seconds apart they should be cleaned
			firstReqTime=lastReqTime;
		}
		EXPECT_EQ(nr,gfc->lastContainer->numRequests);
		EXPECT_EQ(firstReqTime,gfc->lastContainer->firstRequestTime);
		EXPECT_EQ(lastReqTime,gfc->lastContainer->lastRequestTime);
		EXPECT_EQ(nr,gfc->lastContainer->GetFeatureData()[0]);// feature add one should be same as number of requests
	}


}



TEST(bot_banger_aggregator,index_and_feature_event_test)
{
	BotBangerAggregator bag(100,1800);
	FeatureEventListener *listener=new FeatureEventListener(); // listens to the onfeatureevent
	bag.RegisterEventListener(listener);

	bag.RegisterFeature(new FeatureSetValue(9),0);
	bag.RegisterFeature(new FeatureSetValue(8),1);
	bag.RegisterFeature(new FeatureSetValue(7),2);
	bag.RegisterFeature(new FeatureSetValue(6),3);
	// test gap
	bag.RegisterFeature(new FeatureSetValue(5),5);
	bag.RegisterFeature(new FeatureSetValue(4),6);
	bag.RegisterFeature(new FeatureSetValue(3),7);
	bag.RegisterFeature(new FeatureSetValue(2),8);
	bag.RegisterFeature(new FeatureSetValue(1),9);

	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	bag.Aggregate(&le);

	EXPECT_EQ(9,listener->features[0]);
	EXPECT_EQ(8,listener->features[1]);
	EXPECT_EQ(7,listener->features[2]);
	EXPECT_EQ(6,listener->features[3]);
	EXPECT_EQ(0,listener->features[4]); // gap, should be 0
	EXPECT_EQ(5,listener->features[5]);
	EXPECT_EQ(4,listener->features[6]);
	EXPECT_EQ(3,listener->features[7]);
	EXPECT_EQ(2,listener->features[8]);
	EXPECT_EQ(1,listener->features[9]);


	EXPECT_EQ(10,listener->numFeatures);
	EXPECT_STREQ(le.useraddress,listener->key.c_str());
}


