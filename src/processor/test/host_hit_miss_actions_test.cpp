#include <gtest/gtest.h>
#include "../hosthitmiss/host_hit_miss_aggregator.h"
#include "../hosthitmiss/host_hit_miss_actions.h"
#include "../log_entry.h"
#include "log_entry_test.h"
#include <cmath>

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

/* test a request match
 */
TEST(host_hit_miss_actions,test_actions_request_match)
{
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	vector<string> actionList;

	auto actionListener=new GatherActions(actionList);
	actionListener->AddConfigLine(string(le.hostname),string("test1"),0,1999,0.0,1.0,0);
	actionListener->AddConfigLine(string(le.hostname),string("test2"),2000,3000,0.0,1.0,0);

	HostHitMissAggregator hhm(60,5);
	hhm.RegisterEventListener(actionListener);

	for (int c=0;c<3500;c++)
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

		if (c<=3000)
			EXPECT_EQ(1,actionList.size());
		else
			EXPECT_EQ(0,actionList.size());
		if (c==0)
			EXPECT_STREQ((string(le.hostname)+string(":test1:")).c_str(),actionList[0].c_str());
		else
		if (c<2000)
			EXPECT_STREQ((string(le.hostname)+string(":test1:test1")).c_str(),actionList[0].c_str());
		else
		if (c<=3000)
			EXPECT_STREQ((string(le.hostname)+string(":test2:test1")).c_str(),actionList[0].c_str());


		le.endTime+=120; // force next period

		actionList.clear();

	}

}


/* Test a zero ratio matching rule
 */
TEST(host_hit_miss_actions,test_actions_ratio_match_zero)
{
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	strcpy(le.hostname,"host1");

	vector<string> actionList;

	auto actionListener=new GatherActions(actionList);
	// test for ratio 0
	actionListener->AddConfigLine(string(le.hostname),string("zero"),0,9999999,0.0,0.0,0);

	HostHitMissAggregator hhm(60,5);
	hhm.RegisterEventListener(actionListener);

	le.cacheLookupStatus=CacheLookupStatus::Miss;
	hhm.Aggregate(&le);
	EXPECT_EQ(1,actionList.size());
	EXPECT_STREQ("host1:zero:",actionList[0].c_str());
	actionList.clear();

	// add a hit record, we expect the ratio to go up and to not generate an action
	le.endTime+=5;
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	hhm.Aggregate(&le);
	EXPECT_EQ(0,actionList.size());

	// add a miss record, we expect the ratio to go down, but not to zero, so no action
	le.endTime+=5;
	le.cacheLookupStatus=CacheLookupStatus::Miss;
	hhm.Aggregate(&le);
	EXPECT_EQ(0,actionList.size());

	// slide the window past the period, expect an action, because the hit is out of the picture
	le.endTime+=65;
	le.cacheLookupStatus=CacheLookupStatus::Miss;
	hhm.Aggregate(&le);
	EXPECT_EQ(1,actionList.size());
	EXPECT_STREQ("host1:zero:zero",actionList[0].c_str());
	actionList.clear();

}
/* Test a 1 ratio matching rule
 */
TEST(host_hit_miss_actions,test_actions_ratio_match_one)
{
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	strcpy(le.hostname,"host1");

	vector<string> actionList;

	auto actionListener=new GatherActions(actionList);
	// test for ratio 0
	actionListener->AddConfigLine(string(le.hostname),string("one"),0,9999999,1.0,1.0,0);

	HostHitMissAggregator hhm(60,5);
	hhm.RegisterEventListener(actionListener);

	le.cacheLookupStatus=CacheLookupStatus::Hit;
	hhm.Aggregate(&le);
	EXPECT_EQ(1,actionList.size());
	EXPECT_STREQ("host1:one:",actionList[0].c_str());
	actionList.clear();

	// add a miss record, we expect the ratio to go down from 1 and to not generate an action
	le.endTime+=5;
	le.cacheLookupStatus=CacheLookupStatus::Miss;
	hhm.Aggregate(&le);
	EXPECT_EQ(0,actionList.size());

	// add a miss record, we expect the ratio to go up, but not to 1, so no action
	le.endTime+=5;
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	hhm.Aggregate(&le);
	EXPECT_EQ(0,actionList.size());

	// slide the window past the period, expect an action, because the hit is out of the picture
	le.endTime+=65;
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	hhm.Aggregate(&le);
	EXPECT_EQ(1,actionList.size());
	EXPECT_STREQ("host1:one:one",actionList[0].c_str());
	actionList.clear();
}
/* test ratio matching on 0.5
 */
TEST(host_hit_miss_actions,test_actions_ratio_match_half)
{

	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	strcpy(le.hostname,"host1");

	vector<string> actionList;

	auto actionListener=new GatherActions(actionList);
	actionListener->AddConfigLine(string(le.hostname),string("low"),0,9999999,0.0,0.5,0);
	actionListener->AddConfigLine(string(le.hostname),string("high"),0,9999999,0.5,1.0,0);
	HostHitMissAggregator hhm(60,5);
	hhm.RegisterEventListener(actionListener);

	// we will alternate between hit/miss, starting with hit to keep the ratio above .5 if we have a hit
	// and get the ratio on .5 by adding a miss
	for (int c=0;c<100;c++)
	{
		le.endTime+=5;
		bool hit=(c%2) ? false : true;

		le.cacheLookupStatus=hit ? CacheLookupStatus::Hit : CacheLookupStatus::Miss;
		hhm.Aggregate(&le);

		EXPECT_EQ(1,actionList.size());

		if (hit)
		{
			if (c==0)
				EXPECT_STREQ("host1:high:",actionList[0].c_str()); // first has no currentaction
			else
				EXPECT_STREQ("host1:high:low",actionList[0].c_str());
		}
		else
			EXPECT_STREQ("host1:low:high",actionList[0].c_str());
		actionList.clear();
	}

}

/* test runtime of the actions, with the runtime the system will not match the rules for a certain amount of time
 * ater a match
 */
TEST(host_hit_miss_actions,test_actions_runtime_1000)
{
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	strcpy(le.hostname,"host1");

	vector<string> actionList;

	auto actionListener=new GatherActions(actionList);
	// test for runtime 1000
	actionListener->AddConfigLine(string(le.hostname),string("zero"),0,9999999,0.0,1.0,1000);

	HostHitMissAggregator hhm(60,5);
	hhm.RegisterEventListener(actionListener);

	le.cacheLookupStatus=CacheLookupStatus::Miss;
	hhm.Aggregate(&le);
	EXPECT_EQ(1,actionList.size());
	actionList.clear();

	time_t start_time=le.endTime;
	for (int c=0;c<1000;c++)
	{
		le.endTime+=5; // move to next range to trigger actions
		hhm.Aggregate(&le);
		if (actionList.size()) // action matched
		{
			//
			EXPECT_LE(1000,le.endTime-start_time); // is it in the right timeframe
			EXPECT_GT(1010,le.endTime-start_time);
			break;
		}
	}
	EXPECT_EQ(1,actionList.size()); // we should have had a match
}

/* test ratio, request number matrix   */
void test_actions_ratio_request_matrix_inner(int requests,double ratio,int lowerRequest,int upperRequest,double lowerRatio,double upperRatio,bool expect_action)
{
	int hitrequests=(((double) requests)*ratio);

	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	strcpy(le.hostname,"host1");

	vector<string> actionList;

	auto actionListener=new GatherActions(actionList);
	// test for runtime 1000
	actionListener->AddConfigLine(string(le.hostname),string("action"),lowerRequest,upperRequest,lowerRatio,upperRatio,0);
	HostHitMissAggregator hhm(60,5);
	hhm.RegisterEventListener(actionListener);


	int i=0;
	le.cacheLookupStatus=CacheLookupStatus::Hit;
	while (i<hitrequests)
	{
		if (i==(requests-1)) // force an event on the last entry, by sliding to the next range
			le.endTime+=5;
		hhm.Aggregate(&le);
		i++;
	}

	le.cacheLookupStatus=CacheLookupStatus::Miss;
	while (i<requests)
	{
		if (i==(requests-1)) // force an event on the last entry, by sliding to the next range
				le.endTime+=5;
		hhm.Aggregate(&le);
		i++;
	}

	if (expect_action)
	{
		EXPECT_EQ(1,actionList.size());
	}
	else
	{
		EXPECT_EQ(0,actionList.size());
	}


}

void test_actions_ratio_request_matrix(
		int matrixLowerRequest,int matrixUpperRequest,double matrixLowerRatio,double matrixUpperRatio,
		int matchLowerRequest,int matchUpperRequest,double matchLowerRatio,double matchUpperRatio

		)
{
	for(int i=matrixLowerRequest;i<=matrixUpperRequest;i+=10) // test for # of requests 100..1000
	{
		for (double r=matrixLowerRatio;r<=matrixUpperRatio;r+=0.1) // test all ratio from 0 to 1 with an interval
		{
			bool expectAction=(i>=matchLowerRequest) && (i<=matchUpperRequest) && (r>=matchLowerRatio) && (r<=matchUpperRatio); // this range is the matched range, so we expect an action here

			test_actions_ratio_request_matrix_inner(i,r,matchLowerRequest,matchUpperRequest,matchLowerRatio,matchUpperRatio,expectAction);
		}
	}
}

/* test ratio, request number in the matrix cases    */
TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_middle)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			200,300,0.5,0.6);

}

TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_leftupper)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			100,100,0,0);
}

TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_rightupper)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			500,500,0,0);
}

TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_leftlower)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			100,100,1,1);
}

TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_rightlower)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			500,500,1,1);
}

TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_upperedge)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			100,500,0,0);
}

TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_leftedge)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			100,100,0,1);
}

TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_rightedge)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			500,500,0,0);
}

TEST(host_hit_miss_actions,test_actions_ratio_request_matrix_loweredge)
{
	test_actions_ratio_request_matrix(
			100,500,0,1,
			100,500,1,1);
}

