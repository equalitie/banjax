#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_average_time_between_requests.h"


TEST(feature_average_time_between_requests,one_request)
{
	FeatureContainerTest fct(new FeatureAverageTimeBetweenRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(1800,fct.GetFeatureValue()); // one request should give std value
}


TEST(feature_average_time_between_requests,two_requests)
{
	FeatureContainerTest fct(new FeatureAverageTimeBetweenRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(1800,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue());
}

TEST(feature_average_time_between_requests,three_request)
{
	FeatureContainerTest fct(new FeatureAverageTimeBetweenRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(1800,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue());

	le.endTime+=1500;
	fct.Aggregrate(&le);
	EXPECT_EQ(1000,fct.GetFeatureValue());
}

TEST(feature_average_time_between_requests,three_request_outoforder_1)
{
	FeatureContainerTest fct(new FeatureAverageTimeBetweenRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(1800,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue());

	le.endTime-=100;
	fct.Aggregrate(&le);
	EXPECT_EQ(250,fct.GetFeatureValue());
}

TEST(feature_average_time_between_requests,three_request_outoforder_2)
{
	FeatureContainerTest fct(new FeatureAverageTimeBetweenRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(1800,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue());

	le.endTime-=600;
	fct.Aggregrate(&le);
	EXPECT_EQ(250,fct.GetFeatureValue());
}

