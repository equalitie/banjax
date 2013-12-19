#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_request_depth_std.h"

TEST(feature_request_depth_std,same_depth)
{
	FeatureContainerTest fct(new FeatureRequestDepthStd());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2/test.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.url,"/test1/test2/test2.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.url,"/test1/test2/test3.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());



}


TEST(feature_request_depth_std,differ_depth)
{
	FeatureContainerTest fct(new FeatureRequestDepthStd());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2/test.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.url,"/test1/test2.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());

	strcpy(le.url,"/test1/test2/test3.html");
	fct.Aggregrate(&le);
	EXPECT_NEAR(0.471,fct.GetFeatureValue(),0.01);



}



TEST(feature_request_depth_std,differ_depth_add_resources) //resources should never impact feature
{
	FeatureContainerTest fct(new FeatureRequestDepthStd());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2/test.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.contenttype,"text/css");
	strcpy(le.url,"/css/default.css");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/background.jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2.html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());

	strcpy(le.contenttype,"text/css");
	strcpy(le.url,"/css/default.css");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/background.jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());


	strcpy(le.contenttype,"text/html");
	strcpy(le.url,"/test1/test2/test3.html");
	fct.Aggregrate(&le);
	EXPECT_NEAR(0.471,fct.GetFeatureValue(),0.01);


	strcpy(le.contenttype,"text/css");
	strcpy(le.url,"/css/default.css");
	fct.Aggregrate(&le);
	EXPECT_NEAR(0.471,fct.GetFeatureValue(),0.01);

	strcpy(le.contenttype,"image/jpeg");
	strcpy(le.url,"/gfx/background.jpeg");
	fct.Aggregrate(&le);
	EXPECT_NEAR(0.471,fct.GetFeatureValue(),0.01);

}
