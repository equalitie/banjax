#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_html_to_image_ratio.h"


TEST(feature_html_to_image_ratio,one_html)
{
	FeatureContainerTest fct(new FeatureHtmlToImageRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
}


TEST(feature_html_to_image_ratio,one_image)
{
	FeatureContainerTest fct(new FeatureHtmlToImageRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);
	strcpy(le.contenttype,"image/jpeg");

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());
}

TEST(feature_html_to_image_ratio,one_html_one_image)
{
	FeatureContainerTest fct(new FeatureHtmlToImageRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

}

TEST(feature_html_to_image_ratio,one_html_two_image)
{
	FeatureContainerTest fct(new FeatureHtmlToImageRatio());
	LogEntry le;
	strcpy(le.contenttype,"text/html");
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	fct.Aggregrate(&le);
	EXPECT_EQ(2,fct.GetFeatureValue());

}

TEST(feature_html_to_image_ratio,two_html_one_image)
{
	FeatureContainerTest fct(new FeatureHtmlToImageRatio());
	LogEntry le;
	strcpy(le.contenttype,"text/html");
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.contenttype,"text/html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());

}

TEST(feature_html_to_image_ratio,two_html_one_image_other_resources)
{
	FeatureContainerTest fct(new FeatureHtmlToImageRatio());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.contenttype,"text/html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());


	strcpy(le.contenttype,"application/pdf");
	fct.Aggregrate(&le);
	EXPECT_EQ(0,fct.GetFeatureValue());

	strcpy(le.contenttype,"image/jpeg");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.contenttype,"application/binary");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.contenttype,"text/html");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());

	strcpy(le.contenttype,"application/x-word");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());
}

