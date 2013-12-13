#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_http_status_ratio.h"

TEST(feature_http_status_ratio,one_ok)
{
	FeatureContainerTest fct(new FeatureHTTPStatusRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	le.httpCode=200;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
}

TEST(feature_http_status_ratio,one_false)
{
	FeatureContainerTest fct(new FeatureHTTPStatusRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	le.httpCode=404;
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());
}

TEST(feature_http_status_ratio,one_ok_one_false)
{
	FeatureContainerTest fct(new FeatureHTTPStatusRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	le.httpCode=200;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	le.httpCode=404;
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());
}

TEST(feature_http_status_ratio,one_ok_two_false)
{
	FeatureContainerTest fct(new FeatureHTTPStatusRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	le.httpCode=200;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	le.httpCode=404;
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());

	le.httpCode=404;
	fct.Aggregrate(&le);
	EXPECT_EQ(2.0/3.0,fct.GetFeatureValue());
}

TEST(feature_http_status_ratio,two_ok_one_false)
{
	FeatureContainerTest fct(new FeatureHTTPStatusRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	le.httpCode=200;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	le.httpCode=404;
	fct.Aggregrate(&le);
	EXPECT_EQ(1.0/2.0,fct.GetFeatureValue());

	le.httpCode=200;
	fct.Aggregrate(&le);
	EXPECT_EQ(1.0/3.0,fct.GetFeatureValue());
}

TEST(feature_http_status_ratio,two_ok_one_false_other_codes)
{
	FeatureContainerTest fct(new FeatureHTTPStatusRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	le.httpCode=200;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	le.httpCode=500;	// should not change ratio
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	le.httpCode=404;
	fct.Aggregrate(&le);
	EXPECT_EQ(1.0/3.0,fct.GetFeatureValue());

	le.httpCode=200;
	fct.Aggregrate(&le);
	EXPECT_EQ(1.0/4.0,fct.GetFeatureValue());

	le.httpCode=500;	// should not change ratio
	fct.Aggregrate(&le);
	EXPECT_EQ(1.0/5.0,fct.GetFeatureValue());


}

TEST(feature_http_status_ratio,test_all_codes)
{

	for(int i=100;i<999;i++)
	{
		FeatureContainerTest fct(new FeatureHTTPStatusRatio());
		LogEntry le;
		LogEntryTest::InitLogEntry(&le);

		le.httpCode=i;
		fct.Aggregrate(&le);
		if (i>=400 && i<500) // these codes should trigger the ratio
			EXPECT_TRUE(fct.GetFeatureValue()==1);
		else
			EXPECT_TRUE(fct.GetFeatureValue()==0);
	}
}
