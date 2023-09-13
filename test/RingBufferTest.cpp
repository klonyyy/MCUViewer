#include <gtest/gtest.h>

#include <array>

#include "RingBuffer.hpp"

TEST(RingBufferTest, testpushpop)
{
	RingBuffer<std::array<bool, 10>> ringBuffer(2000);

	std::array<bool, 10> array1{0, 1, 0, 0, 1, 1, 0, 1, 1, 0};
	std::array<bool, 10> array2{0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
	std::array<bool, 10> array3{0, 1, 1, 0, 0, 1, 0, 1, 0, 0};

	ringBuffer.push(array1);
	ringBuffer.push(array2);
	ringBuffer.push(array3);

	ASSERT_EQ(ringBuffer.pop(), array1);
	ASSERT_EQ(ringBuffer.pop(), array2);
	ASSERT_EQ(ringBuffer.pop(), array3);
}
