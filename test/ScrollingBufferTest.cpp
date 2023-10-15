#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <numeric>

#include "ScrollingBuffer.hpp"

class ScrollingBufferTest : public ::testing::Test
{
   protected:
	void SetUp() override
	{
		test.setMaxSize(defaultMaxSize);
		std::array<double, defaultMaxSize + 5> data;
		std::iota(data.begin(), data.end(), 1.0);

		for (const auto& element : data)
			test.addPoint(element);

		test.copyData();
	}

	void TearDown() override
	{
	}

	static constexpr uint32_t defaultMaxSize = 20;
	ScrollingBuffer<double> test{};
};

TEST_F(ScrollingBufferTest, testOverflow)
{
	ASSERT_EQ(*(test.getFirstElement()), 21.0);
}

TEST_F(ScrollingBufferTest, testCopy)
{
	for (uint32_t i; i < test.getMaxSize(); i++)
	{
		ASSERT_EQ(*(test.getFirstElement() + i), *(test.getFirstElementCopy() + i));
	}
}

TEST_F(ScrollingBufferTest, testIndexLookup0)
{
	ASSERT_EQ(test.getIndexFromvalue(19), defaultMaxSize - 2);
}

TEST_F(ScrollingBufferTest, testIndexLookup1)
{
	ASSERT_EQ(test.getIndexFromvalue(20), defaultMaxSize - 1);
}

TEST_F(ScrollingBufferTest, testIndexLookup2)
{
	ASSERT_EQ(test.getIndexFromvalue(21), 0);
}

TEST_F(ScrollingBufferTest, testOldestValue)
{
	ASSERT_EQ(test.getOldestValue(), 6);
}

TEST_F(ScrollingBufferTest, testNewestValue)
{
	ASSERT_EQ(test.getNewestValue(), 25);
}

TEST_F(ScrollingBufferTest, testLinearCopy)
{
	auto vec = test.getLinearData(2, 20);
	std::vector<double> result{23, 24, 25, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	ASSERT_EQ(vec, result);
}

TEST_F(ScrollingBufferTest, testLinearCopy2)
{
	auto vec = test.getLinearData(19, 6);
	std::vector<double> result{20, 21, 22, 23, 24, 25, 6};
	ASSERT_EQ(vec, result);
}

TEST_F(ScrollingBufferTest, testLinearCopy3)
{
	auto vec = test.getLinearData(0, 0);
	std::vector<double> result{21, 22, 23, 24, 25, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	ASSERT_EQ(vec, result);
}