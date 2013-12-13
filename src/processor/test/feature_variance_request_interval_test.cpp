#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_variance_request_interval.h"


TEST(feature_variance_request_interval,one_request)
{
	FeatureContainerTest fct(new FeatureVarianceRequestInterval());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // one request should give std value
}

TEST(feature_variance_request_interval,two_requests)
{
	FeatureContainerTest fct(new FeatureVarianceRequestInterval());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // one request should give std value
}

TEST(feature_variance_request_interval,three_requests_nostddev)
{
	FeatureContainerTest fct(new FeatureVarianceRequestInterval());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // one request should give std value
}


TEST(feature_variance_request_interval,three_requests_stddev)
{
	FeatureContainerTest fct(new FeatureVarianceRequestInterval());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=400;
	fct.Aggregrate(&le);
	EXPECT_EQ(50,fct.GetFeatureValue()); // one request should give std value

	le.endTime+=200;
	fct.Aggregrate(&le);
	EXPECT_NEAR(124.722,fct.GetFeatureValue(),0.01);
}
