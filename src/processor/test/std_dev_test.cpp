#include "../utils/std_dev.h"
#include <gtest/gtest.h>


TEST(std_dev,accumulative)
{
	StdDev stddev;
	memset(&stddev,0,sizeof(StdDev));

	stddev.AddCumulative(400);
	EXPECT_EQ(0,stddev.GetStdDevP());
	EXPECT_EQ(0,stddev.GetStdDev());

	stddev.AddCumulative(500);
	EXPECT_EQ(0,stddev.GetStdDevP());
	EXPECT_EQ(0,stddev.GetStdDev());

	stddev.AddCumulative(650);
	EXPECT_EQ(25,stddev.GetStdDevP());
	EXPECT_NEAR(35.3553,stddev.GetStdDev(),0.01);

	stddev.AddCumulative(800);
	EXPECT_NEAR(23.5702,stddev.GetStdDevP(),0.01);
	EXPECT_NEAR(28.86751,stddev.GetStdDev(),0.01);

}

TEST(std_dev,population)
{
	StdDev stddev;
	memset(&stddev,0,sizeof(StdDev));

	stddev.AddNum(100);
	EXPECT_EQ(0,stddev.GetStdDevP());
	EXPECT_EQ(0,stddev.GetStdDev());

	stddev.AddNum(150);
	EXPECT_EQ(25,stddev.GetStdDevP());
	EXPECT_NEAR(35.3553,stddev.GetStdDev(),0.01);

	stddev.AddNum(150);
	EXPECT_NEAR(23.5702,stddev.GetStdDevP(),0.01);
	EXPECT_NEAR(28.86751,stddev.GetStdDev(),0.01);

}

