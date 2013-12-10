#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_cycling_user_agent.h"

TEST(feature_cycling_user_agent,no_tests_implemented_yet)
{
	FeatureContainerTest fct(new FeatureCyclingUserAgent());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(1,0);
}
