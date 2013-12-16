#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_percentage_consecutive_requests.h"

TEST(feature_percentage_consecutive_requests,same_path)
{
	FeatureContainerTest fct(new FeaturePercentageConsecutiveRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.url,"/test1/test2/test.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?


	strcpy(le.url,"/test1/test2/test2.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue()); // first consecutive request


	strcpy(le.url,"/test1/test2/test3.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(2.0/3.0,fct.GetFeatureValue());


	strcpy(le.url,"/test1/test2/test3.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(3.0/4.0,fct.GetFeatureValue());
}


TEST(feature_percentage_consecutive_requests,change_once)
{
	FeatureContainerTest fct(new FeaturePercentageConsecutiveRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.url,"/test1/test2/test.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?


	strcpy(le.url,"/test1/test2/test2.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue()); // first consecutive request


	strcpy(le.url,"/test1/test2/test3.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(2.0/3.0,fct.GetFeatureValue());


	strcpy(le.url,"/test1/test2/test4.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(3.0/4.0,fct.GetFeatureValue());


	strcpy(le.url,"/test1/test3/test.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(3.0/5.0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?


	strcpy(le.url,"/test1/test3/test2.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(4.0/6.0,fct.GetFeatureValue()); // first consecutive request


	strcpy(le.url,"/test1/test3/test3.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(5.0/7.0,fct.GetFeatureValue());


	strcpy(le.url,"/test1/test3/test4.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(6.0/8.0,fct.GetFeatureValue());

}


TEST(feature_percentage_consecutive_requests,same_path_other_resources)
{
	FeatureContainerTest fct(new FeaturePercentageConsecutiveRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2/test.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?

	strcpy(le.contenttype,"text/css");
	strcpy(le.url,"/css/default.css");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/background.jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?


	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2/test2.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue()); // first consecutive request


	strcpy(le.contenttype,"text/css");
	strcpy(le.url,"/css/default.css");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue()); // first request is not consecutive, is this OK?

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/background.jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue()); // first request is not consecutive, is this OK?

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2/test3.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(2.0/3.0,fct.GetFeatureValue());

	strcpy(le.contenttype,"text/css");
	strcpy(le.url,"/css/default.css");
	fct.Aggregrate(&le);
	EXPECT_EQ(2.0/3.0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/background.jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(2.0/3.0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2/test3.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(3.0/4.0,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/background.jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(3.0/4.0,fct.GetFeatureValue()); // first request is not consecutive, is this OK?
}

