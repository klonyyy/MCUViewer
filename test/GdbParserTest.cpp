#include <gtest/gtest.h>

#include <memory>

#include "GdbParser.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

TEST(GdbParserTest, test)
{
	// auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	// auto logger = std::make_shared<spdlog::logger>("logger", stdout_sink);
	// spdlog::register_logger(logger);

	// GdbParser parser(logger.get());
	// auto result = parser.parse("./test/MCUViewer_test.elf");

	// auto parsedData = parser.getParsedData();

	// for (auto& entry : parsedData)
	// 	std::cout << entry.first << std::endl;

	ASSERT_EQ(true, true);
}