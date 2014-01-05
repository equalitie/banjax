#include <gtest/gtest.h>
#include "../hosthitmiss/host_hit_miss_aggregator.h"
#include "../hosthitmiss/host_hit_miss_actions.h"
#include "../log_entry.h"
#include "log_entry_test.h"

class GatherActions:public HostHitMissActions
{
	vector<string> &_actionList;
public:
	GatherActions(vector<string> &actionList):HostHitMissActions(),_actionList(actionList)
	{

	}
	virtual void OnScheduleAction(HitMissRange *hmr,string &host,string &action,string &currentaction)
	{
		UNUSED(hmr);
		_actionList.push_back(
				host+string(":")+
				action+string(":")+
				currentaction
				);
	}
};

TEST(host_hit_miss_actions,test_actions_request_match)
{
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	vector<string> actionList;

	auto actionListener=new GatherActions(actionList);
	actionListener->AddConfigLine(string(le.hostname),string("test1"),0,1999,0.0,1.0,0);
	actionListener->AddConfigLine(string(le.hostname),string("test2"),2000,5000,0.0,1.0,0);

	HostHitMissAggregator hhm(60,5);
	hhm.RegisterEventListener(actionListener);

	for (int c=0;c<6000;c++)
	{
		for (int i=0;i<(c-1);i++)
		{
			hhm.Aggregate(&le);
			if (i==0) // we triggered a new range
			{
				EXPECT_EQ(1,actionList.size());
				actionList.clear();

			}
		}
		le.endTime+=5; // force next range
		hhm.Aggregate(&le);

		if (actionList.size()==2)
		{
			std::cout << actionList[0] <<":"<< actionList[1] <<std::endl;
		}

		if (c<=5000)
			EXPECT_EQ(1,actionList.size());
		else
			EXPECT_EQ(0,actionList.size());
		if (c==0)
			EXPECT_STREQ((string(le.hostname)+string(":test1:")).c_str(),actionList[0].c_str());
		else
		if (c<2000)
			EXPECT_STREQ((string(le.hostname)+string(":test1:test1")).c_str(),actionList[0].c_str());
		else
		if (c<=5000)
			EXPECT_STREQ((string(le.hostname)+string(":test2:test1")).c_str(),actionList[0].c_str());


		le.endTime+=120; // force next period

		actionList.clear();

	}

}


TEST(host_hit_miss_actions,test_actions_ratio_match)
{
	/*LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	vector<string> actionList;

	auto actionListener=new GatherActions(actionList);
	actionListener->AddConfigLine(string(le.hostname),string("test1"),0,9999999,0.0,0.5,0);
	actionListener->AddConfigLine(string(le.hostname),string("test2"),0,9999999,0.5,1.0,0);
	HostHitMissAggregator hhm(60,5);
	hhm.RegisterEventListener(actionListener);


	for (int c=0;c<6000;c++)
	{

	}*/

}


TEST(host_hit_miss_actions,test_actions_runtime)
{

}

TEST(host_hit_miss_actions,test_actions_fullmatch)
{

}
