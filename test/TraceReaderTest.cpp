#include <gtest/gtest.h>

#include <memory>

#include "ITraceDevice.hpp"
#include "TraceReader/TraceReader.hpp"
#include "gmock/gmock.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

using namespace testing;
using testing::InSequence;

class TraceDeviceMock : public ITraceDevice
{
   public:
	TraceDeviceMock(){};

	MOCK_METHOD(bool, startTrace, (uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask), (override));
	MOCK_METHOD(bool, stopTrace, (), (override));
	MOCK_METHOD(int32_t, readTraceBuffer, (uint8_t * buffer, uint32_t size), (override));
};

class TraceReaderTest : public ::testing::Test
{
   protected:
	TraceReaderTest()
	{
		stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		logger = std::make_shared<spdlog::logger>("logger", stdout_sink);
		spdlog::register_logger(logger);
		traceDevice = std::make_shared<::NiceMock<TraceDeviceMock>>();
		traceReader = std::make_shared<TraceReader>(traceDevice, logger);
	}
	void SetUp() override
	{
		ON_CALL(*traceDevice, startTrace(_, _, _)).WillByDefault(Return(true));

		for (auto& el : activeChannels)
			el = true;
	}
	void TearDown() override
	{
		spdlog::shutdown();
		traceReader->stopAcqusition();
	}
	static constexpr uint32_t channels = 10;
	std::array<bool, 32> activeChannels{};
	std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdout_sink;
	std::shared_ptr<spdlog::logger> logger;

	std::shared_ptr<TraceReader> traceReader;
	std::shared_ptr<::NiceMock<TraceDeviceMock>> traceDevice;
};

TEST_F(TraceReaderTest, startTest)
{
	ASSERT_EQ(traceReader->startAcqusition(activeChannels), true);
}

TEST_F(TraceReaderTest, testChannelsAndTimestamp)
{
	uint8_t buf[] = {9, 187, 192, 206, 9};
	std::array<uint32_t, 10> trace{};
	std::array<uint32_t, 10> expected{0, 187, 0, 0, 0, 0, 0, 0, 0, 0};
	double timestamp = 0.0;

	EXPECT_CALL(*traceDevice, readTraceBuffer(_, _)).WillOnce(testing::Invoke([&](uint8_t* buffer, uint32_t size)
																			  {memcpy(buffer,buf,sizeof(buf));
                                                                                return sizeof(buf); }))
		.WillRepeatedly(Return(0));

	traceReader->startAcqusition(activeChannels);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	ASSERT_EQ(traceReader->readTrace(timestamp, trace), true);
	ASSERT_NEAR(7.6875e-06, timestamp, 1e-9);
	ASSERT_EQ(trace, expected);
}

TEST_F(TraceReaderTest, testChannelsAndTimestamp2)
{
	uint8_t buf[] = {9, 187, 192, 206, 9,
					 17, 170, 192, 35,
					 17, 187, 192, 233, 2,
					 9, 170, 192, 165, 1,
					 9, 187, 192, 202, 9,
					 17, 170, 192, 35,
					 17, 187, 192, 234, 2,
					 25, 170, 192, 23};

	std::array<uint32_t, channels> trace{};

	std::array<double, 10> expectedTimestamp = {7.6875e-06, 1.25e-08, 2.25625e-06, 1.03125e-06, 7.6625e-06, 1.25e-08, 2.2625e-06};
	std::array<std::array<uint32_t, channels>, 8> expectedTrace{{{{0, 187, 0, 0, 0, 0, 0, 0, 0, 0}},
																 {{0, 187, 170, 0, 0, 0, 0, 0, 0, 0}},
																 {{0, 187, 187, 0, 0, 0, 0, 0, 0, 0}},
																 {{0, 170, 187, 0, 0, 0, 0, 0, 0, 0}},
																 {{0, 187, 187, 0, 0, 0, 0, 0, 0, 0}},
																 {{0, 187, 170, 0, 0, 0, 0, 0, 0, 0}},
																 {{0, 187, 187, 0, 0, 0, 0, 0, 0, 0}},
																 {{0, 187, 187, 170, 0, 0, 0, 0, 0, 0}}}};

	EXPECT_CALL(*traceDevice, readTraceBuffer(_, _))
		.WillOnce(testing::Invoke([&](uint8_t* buffer, uint32_t size)
								  {memcpy(buffer,buf,sizeof(buf));
                                   return sizeof(buf); }))
		.WillRepeatedly(Return(0));

	traceReader->startAcqusition(activeChannels);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	int i = 0;
	for (auto& e : expectedTrace)
	{
		double timestamp = 0.0;
		ASSERT_EQ(traceReader->readTrace(timestamp, trace), true);
		ASSERT_NEAR(expectedTimestamp[i++], timestamp, 10e-9);
		ASSERT_EQ(trace, e);
	}
}
