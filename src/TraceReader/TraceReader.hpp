#ifndef _ITRACEREADER_HPP
#define _ITRACEREADER_HPP

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ITraceDevice.hpp"
#include "RingBuffer.hpp"
#include "spdlog/spdlog.h"

class TraceReader
{
   public:
	struct TraceIndicators
	{
		uint32_t framesTotal;
		uint32_t errorFramesTotal;
		uint32_t errorFramesInView;
		uint32_t delayedTimestamp1;
		uint32_t delayedTimestamp2;
		uint32_t delayedTimestamp3;
		uint32_t delayedTimestamp3InView;
		uint32_t sleepCycles;
	};

	TraceReader(spdlog::logger* logger);

	bool startAcqusition(const std::array<bool, 32>& activeChannels);
	bool stopAcqusition();
	bool isValid() const;

	bool readTrace(double& timestamp, std::array<uint32_t, 10>& trace);

	std::string getLastErrorMsg() const;

	void setCoreClockFrequency(uint32_t frequencyHz);
	uint32_t getCoreClockFrequency() const;
	void setTraceFrequency(uint32_t frequencyHz);
	uint32_t getTraceFrequency() const;
	void setTraceShouldReset(bool shouldReset);
	void setTraceTimeout(uint32_t timeout);

	std::vector<std::string> getConnectedDevices() const;
	void changeDevice(std::shared_ptr<ITraceDevice> newTraceDevice);
	std::string getTargetName();

	TraceIndicators getTraceIndicators() const;

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
	TraceIndicators traceIndicators{};

	static constexpr uint32_t channels = 10;
	static constexpr uint32_t size = 10 * 2048;
	uint8_t buffer[size]{};

	uint32_t currentValue[channels]{};
	uint8_t currentChannel[channels]{};

	uint8_t awaitingTimestamp = 0;
	uint32_t timestamp = 0;
	uint8_t sourceFrameSize = 0;

	std::vector<uint8_t> timestampVec;

	uint32_t coreFrequency = 160000;
	uint32_t tracePrescaler = 10;
	uint32_t traceTimeout = 2;
	bool shouldReset = false;

	std::atomic<bool> isRunning{false};
	std::string lastErrorMsg = "";
	std::array<uint32_t, channels> previousEntry{};
	RingBuffer<std::pair<std::array<uint32_t, channels>, double>, 2000> traceTable;
	std::thread readerHandle;

	std::shared_ptr<ITraceDevice> traceDevice;
	spdlog::logger* logger;
	mutable std::mutex mtx;

	TraceState updateTraceIdle(uint8_t c);
	TraceState updateTrace(uint8_t c);
	void timestampEnd(bool headerData);
	void readerThread();
};

#endif