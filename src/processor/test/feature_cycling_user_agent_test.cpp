#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_cycling_user_agent.h"

TEST(feature_cycling_user_agent,one_browser_one)
{
	FeatureContainerTest fct(new FeatureCyclingUserAgent());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.userAgent,"browser 1");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.userAgent,"browser 1");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());
}



TEST(feature_cycling_user_agent,two_browsers)
{
	FeatureContainerTest fct(new FeatureCyclingUserAgent());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	strcpy(le.userAgent,"browser 1");
	fct.Aggregrate(&le);
	EXPECT_EQ(1,fct.GetFeatureValue());

	strcpy(le.userAgent,"browser 2");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());

	strcpy(le.userAgent,"browser 1");
	fct.Aggregrate(&le);
	EXPECT_NEAR(0.66667,fct.GetFeatureValue(),0.01);

	strcpy(le.userAgent,"browser 2");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.5,fct.GetFeatureValue());

	strcpy(le.userAgent,"browser 2");
	fct.Aggregrate(&le);
	EXPECT_EQ(0.6,fct.GetFeatureValue());

	strcpy(le.userAgent,"browser 2");
	fct.Aggregrate(&le);
	EXPECT_NEAR(0.66667,fct.GetFeatureValue(),0.01);

}

TEST(feature_cycling_user_agent,MAX_USER_AGENT_TESTS_SAME)
{
	FeatureContainerTest fct(new FeatureCyclingUserAgent());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	for (int c=0;c<MAX_USER_AGENTS*2;c++) // same distribution
	{
		sprintf(le.userAgent,"browser %d",c);
		fct.Aggregrate(&le);
		EXPECT_NEAR(1.0/((double)(c+1) ),fct.GetFeatureValue(),0.0001);
	}


}

TEST(feature_cycling_user_agent,MAX_USER_AGENT_TESTS_SAME_DIST_ADD_LOWER)
{
	FeatureContainerTest fct(new FeatureCyclingUserAgent());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	for (int c=0;c<MAX_USER_AGENTS;c++) // same distribution
	{
		sprintf(le.userAgent,"browser %d",c);
		fct.Aggregrate(&le);
		EXPECT_NEAR(1.0/((double)(c+1) ),fct.GetFeatureValue(),0.0001);
	}

	strcpy(le.userAgent,"browser 0"); // only add to first browser entry, feature must follow
	for (int c=0;c<200;c++)
	{
		fct.Aggregrate(&le);
		EXPECT_NEAR((double) (c+2)/ (double) (MAX_USER_AGENTS+c+1),fct.GetFeatureValue(),0.0001);
	}
}

TEST(feature_cycling_user_agent,MAX_USER_AGENT_TESTS_SAME_DIST_ADD_HIGHER)
{
	FeatureContainerTest fct(new FeatureCyclingUserAgent());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	for (int c=0;c<MAX_USER_AGENTS;c++) // same distribution
	{
		sprintf(le.userAgent,"browser %d",c);
		fct.Aggregrate(&le);
		EXPECT_NEAR(1.0/((double)(c+1) ),fct.GetFeatureValue(),0.0001);
	}
	// only add to last browser entry, must screw up stats, as we cannot keep
	// keep stats above max_user_agents
	sprintf(le.userAgent,"browser %d",MAX_USER_AGENTS-1);
	for (int c=0;c<200;c++)
	{
		fct.Aggregrate(&le);
		// feature does not see newly added records, they're above the
		// threshold
		EXPECT_NEAR((double) (1)/ (double) (MAX_USER_AGENTS+c+1),fct.GetFeatureValue(),0.0001);
	}
}

