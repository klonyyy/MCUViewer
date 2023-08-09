#ifndef _ITRACEREADER_HPP
#define _ITRACEREADER_HPP

#include <string>
#include <vector>

#include "ITraceDevice.hpp"
#include "RingBuffer.hpp"
#include "ScrollingBuffer.hpp"
#include "spdlog/spdlog.h"

class TraceReader
{
   public:
	TraceReader(std::shared_ptr<ITraceDevice> traceDevice, std::shared_ptr<spdlog::logger> logger);

	bool startAcqusition();
	bool stopAcqusition();
	bool isValid() const;

	bool readTrace(double& timestamp, std::array<bool, 10>& trace);

	std::string getLastErrorMsg() const;

	void setCoreClockFrequency(uint32_t frequencyHz);
	uint32_t getCoreClockFrequency() const;
	void setTraceFrequency(uint32_t frequencyHz);
	uint32_t getTraceFrequency() const;

   private:
	typedef enum
	{
		TRACE_STATE_UNKNOWN,
		TRACE_STATE_IDLE,
		TRACE_STATE_TARGET_SOURCE,
		TRACE_STATE_TARGET_TIMESTAMP_HEADER,
		TRACE_STATE_TARGET_TIMESTAMP_CONT,
		TRACE_STATE_TARGET_TIMESTAMP_END,
		TRACE_STATE_SKIP_FRAME,
		TRACE_STATE_SKIP_4,
		TRACE_STATE_SKIP_3,
		TRACE_STATE_SKIP_2,
		TRACE_STATE_SKIP_1,
	} TraceState;

	typedef struct
	{
		uint32_t errorFrames;
		uint32_t delayedTimestamp1;
		uint32_t delayedTimestamp2;
		uint32_t delayedTimestamp3;
	} TraceQuality;

	TraceState state = TRACE_STATE_IDLE;

	static constexpr uint32_t channels = 10;

	uint8_t currentValue[5]{};
	uint8_t awaitingTimestamp = 0;
	uint8_t currentChannel[5]{};
	uint8_t timestampBuf[7]{};
	uint32_t timestampBytes;
	uint32_t timestamp;
	uint32_t errorCount;
	uint32_t coreFrequency = 160000000;
	uint32_t traceFrequency = 16000000;

	bool isRunning = false;
	std::string lastErrorMsg = "";

	std::array<bool, channels> previousEntry;
	RingBuffer<std::pair<std::array<bool, channels>, uint32_t>> traceTable{200000};

	std::thread readerHandle;

	TraceState updateTraceIdle(uint8_t c);
	TraceState updateTrace(uint8_t c);
	void timestampEnd();

	void readerThread();

	std::shared_ptr<ITraceDevice> traceDevice;
	std::shared_ptr<spdlog::logger> logger;
};

#endif