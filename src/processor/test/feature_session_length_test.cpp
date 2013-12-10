#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_session_length.h"


TEST(feature_session_length,one_value)
{
	FeatureContainerTest fct(new FeatureSessionLength());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
}


TEST(feature_session_length,two_values)
{
	FeatureContainerTest fct(new FeatureSessionLength());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue());
}

TEST(feature_session_length,three_values)
{
	FeatureContainerTest fct(new FeatureSessionLength());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue());
	le.endTime+=250;
	fct.Aggregrate(&le);
	EXPECT_EQ(750,fct.GetFeatureValue());
}


TEST(feature_session_length,three_values_out_of_order_1)
{
	FeatureContainerTest fct(new FeatureSessionLength());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue());
	le.endTime-=100;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue()); // can never be out of bounds
}


TEST(feature_session_length,three_values_out_of_order_2)
{
	FeatureContainerTest fct(new FeatureSessionLength());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
	le.endTime+=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue());
	le.endTime-=600; 			// place it before the first request
	fct.Aggregrate(&le);
	EXPECT_EQ(500,fct.GetFeatureValue()); // can never be out of bounds
}
