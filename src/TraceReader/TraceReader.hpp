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

	bool readTrace(double& timestamp, std::array<double, 10>& trace);

	std::string getLastErrorMsg() const;

	void setCoreClockFrequency(uint32_t frequencyHz);
	uint32_t getCoreClockFrequency() const;
	void setTraceFrequency(uint32_t frequencyHz);
	uint32_t getTraceFrequency() const;
	std::map<const char*, uint32_t> getTraceIndicators() const;

   private:
	typedef enum
	{
		TRACE_STATE_UNKNOWN,
		TRACE_STATE_IDLE,
		TRACE_STATE_TARGET_SOURCE,
		TRACE_STATE_TARGET_SOURCE_3B,
		TRACE_STATE_TARGET_SOURCE_2B,
		TRACE_STATE_TARGET_SOURCE_1B,
		TRACE_STATE_TARGET_SOURCE_0B,
		TRACE_STATE_TARGET_TIMESTAMP_HEADER,
		TRACE_STATE_TARGET_TIMESTAMP_CONT,
		TRACE_STATE_TARGET_TIMESTAMP_END,
		TRACE_STATE_SKIP_FRAME,
		TRACE_STATE_SKIP_4,
		TRACE_STATE_SKIP_3,
		TRACE_STATE_SKIP_2,
		TRACE_STATE_SKIP_1,
	} TraceState;

	TraceState state = TRACE_STATE_IDLE;

	std::map<const char*, uint32_t> traceQuality{{"error frames", 0},
												 {"delayed timestamp 1", 0},
												 {"delayed timestamp 2", 0},
												 {"delayed timestamp 3", 0}};

	static constexpr uint32_t channels = 10;

	/* make sure this value is the same as in stlink library */
	static constexpr uint32_t size = 10 * 2048;
	uint8_t buffer[size];

	uint32_t currentValue[channels]{};
	uint8_t awaitingTimestamp = 0;
	uint8_t currentChannel[channels]{};
	uint8_t timestampBuf[7]{};
	uint32_t timestampBytes = 0;
	uint32_t timestamp;

	uint32_t sleepCycles = 0;

	uint32_t coreFrequency = 160000;
	uint32_t traceFrequency = 10;

	std::atomic<bool> isRunning{false};
	std::string lastErrorMsg = "";

	std::array<double, channels> previousEntry;
	RingBuffer<std::pair<std::array<double, channels>, uint32_t>> traceTable{200000};

	std::thread readerHandle;

	TraceState updateTraceIdle(uint8_t c);
	TraceState updateTrace(uint8_t c);
	void timestampEnd();

	void readerThread();

	std::shared_ptr<ITraceDevice> traceDevice;
	std::shared_ptr<spdlog::logger> logger;
};

#endif