#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_request_depth_std.h"

TEST(feature_request_depth_std,no_tests_implemented_yet)
{
	FeatureContainerTest fct(new FeatureRequestDepthStd());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(1,0);
}
