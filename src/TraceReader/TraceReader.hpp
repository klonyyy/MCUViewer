#ifndef _ITRACEREADER_HPP
#define _ITRACEREADER_HPP

#include <string>
#include <vector>

#include "ITraceDevice.hpp"
#include "RingBuffer.hpp"
#include "map"
#include "spdlog/spdlog.h"

class TraceReader
{
   public:
	TraceReader(std::shared_ptr<ITraceDevice> traceDevice, std::shared_ptr<spdlog::logger> logger);

	bool startAcqusition(std::array<bool, 32>& activeChannels);
	bool stopAcqusition();
	bool isValid() const;

	bool readTrace(double& timestamp, std::array<uint32_t, 10>& trace);

	std::string getLastErrorMsg() const;

	void setCoreClockFrequency(uint32_t frequencyHz);
	uint32_t getCoreClockFrequency() const;
	void setTraceFrequency(uint32_t frequencyHz);
	uint32_t getTraceFrequency() const;
	std::map<std::string, uint32_t> getTraceIndicators() const;

   private:
	typedef enum
	{
		TRACE_STATE_UNKNOWN,
		TRACE_STATE_IDLE,
		TRACE_STATE_TARGET_SOURCE_1B,
		TRACE_STATE_TARGET_SOURCE_2B,
		TRACE_STATE_TARGET_SOURCE_3B,
		TRACE_STATE_TARGET_SOURCE_4B,
		TRACE_STATE_TARGET_TIMESTAMP_HEADER,
		TRACE_STATE_TARGET_TIMESTAMP_CONT,
		TRACE_STATE_TARGET_TIMESTAMP_END,
		TRACE_STATE_SKIP_FRAME,
	} TraceState;

	TraceState state = TRACE_STATE_IDLE;

	std::map<std::string, uint32_t> traceQuality{{"frames total", 0},
												 {"error frames total", 0},
												 {"error frames in view", 0},
												 {"delayed timestamp 1", 0},
												 {"delayed timestamp 2", 0},
												 {"delayed timestamp 3", 0},
												 {"sleep cycles", 0}};

	static constexpr uint32_t channels = 10;
	static constexpr uint32_t size = 10 * 2048;
	uint8_t buffer[size];

	uint32_t currentValue[channels]{};
	uint8_t currentChannel[channels]{};

	uint8_t awaitingTimestamp = 0;
	uint32_t timestamp = 0;
	uint8_t sourceFrameSize = 0;

	std::vector<uint8_t> timestampVec;

	uint32_t coreFrequency = 160000;
	uint32_t tracePrescaler = 10;

	std::atomic<bool> isRunning{false};
	std::string lastErrorMsg = "";
	std::array<uint32_t, channels> previousEntry{};
	std::unique_ptr<RingBuffer<std::pair<std::array<uint32_t, channels>, double>>> traceTable;
	std::thread readerHandle;
	std::shared_ptr<ITraceDevice> traceDevice;
	std::shared_ptr<spdlog::logger> logger;

	TraceState updateTraceIdle(uint8_t c);
	TraceState updateTrace(uint8_t c);
	void timestampEnd(bool headerData);
	void readerThread();
};

#endif