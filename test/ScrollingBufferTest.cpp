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
		std::array<double, defaultMaxSize + 5> data;
		std::iota(data.begin(), data.end(), 1.0);

		for (const auto& element : data)
			test.addPoint(element);

		test.copyData();
	}

	void TearDown() override
	{
	}

	static constexpr uint32_t defaultMaxSize = 10000;
	ScrollingBuffer<double> test{};
};

TEST_F(ScrollingBufferTest, testOverflow)
{
	ASSERT_EQ(*(test.getFirstElement()), 10001.0);
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
	ASSERT_EQ(test.getIndexFromvalue(9999), defaultMaxSize - 2);
}

TEST_F(ScrollingBufferTest, testIndexLookup1)
{
	ASSERT_EQ(test.getIndexFromvalue(10000), defaultMaxSize - 1);
}

TEST_F(ScrollingBufferTest, testIndexLookup2)
{
	ASSERT_EQ(test.getIndexFromvalue(10001), 0);
}

TEST_F(ScrollingBufferTest, testOldestValue)
{
	ASSERT_EQ(test.getOldestValue(), 6);
}

TEST_F(ScrollingBufferTest, testNewestValue)
{
	ASSERT_EQ(test.getNewestValue(), 10005);
}