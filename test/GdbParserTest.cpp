#include <gtest/gtest.h>

#include "GdbParser.hpp"

TEST(GdbParserTest, test)
{
	GdbParser parser;
	parser.parse("C:/Users/klonyyy/PROJECTS/STMViewer_/STMViewer/test/STMViewer_test/Debug/STMViewer_test.elf");

	ASSERT_EQ(1, 1);
}