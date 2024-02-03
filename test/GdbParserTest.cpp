#include <gtest/gtest.h>

#include <memory>

#include "GdbParser.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

TEST(GdbParserTest, test)
{
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto logger = std::make_shared<spdlog::logger>("logger", stdout_sink);
	spdlog::register_logger(logger);

	GdbParser parser(logger.get());
	auto result = parser.parse("C:/Users/klonyyy/PROJECTS/STMViewer_/STMViewer/test/testFiles/STMViewer_test.elf");

	auto parsedData = parser.getParsedData();

	for (auto& entry : parsedData)
		std::cout << entry.name << std::endl;

	ASSERT_EQ(result, true);
}