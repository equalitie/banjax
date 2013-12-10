#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_average_payload_size.h"





TEST(feature_average_payload_size,one_value)
{
	FeatureContainerTest fct(new FeatureAveragePayloadSize());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(512,fct.GetFeatureValue());
}

TEST(feature_average_payload_size,two_same_values)
{
	FeatureContainerTest fct(new FeatureAveragePayloadSize());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(512,fct.GetFeatureValue());

	fct.Aggregrate(&le);
	EXPECT_EQ(512,fct.GetFeatureValue());

}

TEST(feature_average_payload_size,two_different_values)
{
	FeatureContainerTest fct(new FeatureAveragePayloadSize());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(512,fct.GetFeatureValue());
	le.payloadsize=1024;
	fct.Aggregrate(&le);
	EXPECT_EQ(768,fct.GetFeatureValue());
}

TEST(feature_average_payload_size,three_different_values)
{
	FeatureContainerTest fct(new FeatureAveragePayloadSize());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	le.payloadsize=0;
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
	le.payloadsize=500;
	fct.Aggregrate(&le);
	EXPECT_EQ(250,fct.GetFeatureValue());
	le.payloadsize=700;
	fct.Aggregrate(&le);
	EXPECT_EQ(400,fct.GetFeatureValue());
}
