#include <gtest/gtest.h>

#include <vector>
#define TEST_FRIENDS_STATISTICS friend class StatisticsTest;
#include "Statistics.hpp"

class StatisticsTest : public ::testing::Test
{
   protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}

   public:
	static void convertDigitalSeriesToVectors(std::vector<double> time, std::vector<double> data, std::vector<double>& Lvec, std::vector<double>& Hvec)
	{
		Statistics::convertDigitalSeriesToVectors(time, data, std::forward<std::vector<double>&>(Lvec), std::forward<std::vector<double>&>(Hvec));
	}
};

TEST(StatisticsTest, testLvecHvec)
{
	std::vector<double> time{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
	std::vector<double> data{0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0};
	std::vector<double> Lvec, Hvec;

	StatisticsTest::convertDigitalSeriesToVectors(time, data, Lvec, Hvec);

	for (auto& a : Lvec)
		std::cout << a << " ";
	std::cout << std::endl;

	for (auto& a : Hvec)
		std::cout << a << " ";
	std::cout << std::endl;
}

TEST(StatisticsTest, testLvecHvec2)
{
	std::vector<double> time{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
	std::vector<double> data{0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0};
	std::vector<double> Lvec, Hvec;

	StatisticsTest::convertDigitalSeriesToVectors(time, data, Lvec, Hvec);

	for (auto& a : Lvec)
		std::cout << a << " ";
	std::cout << std::endl;

	for (auto& a : Hvec)
		std::cout << a << " ";
	std::cout << std::endl;
}
