#include <gtest/gtest.h>
#include "log_entry_test.h"
#include "feature_container_test.h"
#include "../features/feature_percentage_consecutive_requests.h"

TEST(feature_percentage_consecutive_requests,no_tests_implemented_yet)
{
	FeatureContainerTest fct(new FeaturePercentageConsecutiveRequests());
	LogEntry le;
	LogEntryTest::InitLogEntry(&le);

	fct.Aggregrate(&le);
	EXPECT_EQ(1,0);
}
