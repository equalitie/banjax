#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_request_depth.h"


TEST(feature_request_depth,zero_deep)
{
	FeatureContainerTest fct(new FeatureRequestDepth());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue()); // no url is 1 deep, zero should not be possible
}


TEST(feature_request_depth,one_deep)
{
	FeatureContainerTest fct(new FeatureRequestDepth());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/sub");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());
}

TEST(feature_request_depth,two_deep)
{
	FeatureContainerTest fct(new FeatureRequestDepth());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/sub/sub");
	fct.Aggregrate(&le);
	EXPECT_EQ(2,fct.GetFeatureValue());
}

TEST(feature_request_depth,one_and_two_deep)
{
	FeatureContainerTest fct(new FeatureRequestDepth());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/sub");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.url,"/sub/sub");
	fct.Aggregrate(&le);
	EXPECT_EQ(1.5,fct.GetFeatureValue());
}

TEST(feature_request_depth,one_and_two_deep_add_nonhtml)
{
	FeatureContainerTest fct(new FeatureRequestDepth());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/sub");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/blank.jpg");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.contenttype,"text/css");
	strcpy(le.url,"/css/default.css");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/sub/sub");
	fct.Aggregrate(&le);
	EXPECT_EQ(1.5,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/blank.jpg");
	fct.Aggregrate(&le);
	EXPECT_EQ(1.5,fct.GetFeatureValue());
}


TEST(feature_request_depth,two_multi_very_deep_very)
{
	FeatureContainerTest fct(new FeatureRequestDepth());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/sub/sub/sub/sub/sub/sub/sub/sub");
	fct.Aggregrate(&le);
	EXPECT_EQ(8,fct.GetFeatureValue());

	strcpy(le.url,"/sub/sub/sub/sub/sub/sub/sub/sub/sub/sub");
	fct.Aggregrate(&le);
	EXPECT_EQ(9,fct.GetFeatureValue());
}
