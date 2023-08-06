#ifndef _STLINKTRACEREADER_HPP
#define _STLINKTRACEREADER_HPP

#include <thread>

#include "ITraceReader.hpp"
#include "RingBuffer.hpp"
#include "stlink.h"

class StlinkTraceReader : public ITraceReader
{
   public:
	StlinkTraceReader();
	bool startAcqusition() override;
	bool stopAcqusition() override;
	bool isValid() const override;

	bool readTrace(double& timestamp, std::array<bool, 10>& trace) override;

	std::string getLastErrorMsg() const override;

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

	TraceState state = TRACE_STATE_IDLE;

	static constexpr uint32_t channels = 10;

	uint8_t currentValue = 0;
	uint8_t currentChannel = 0;
	uint8_t timestampBuf[7]{};
	uint32_t timestampBytes;
	uint32_t timestamp;
	uint32_t errorCount;
	uint32_t coreFrequency = 160000000;
	uint32_t traceFrequency = 16000000;

	stlink_t* sl = nullptr;
	bool isRunning = false;
	std::string lastErrorMsg = "";

	std::thread readerHandle;
	void readerThread();

	bool enableTrace();
	TraceState updateTraceIdle(uint8_t c);
	TraceState updateTrace(uint8_t c);
	void timestampEnd();

	uint32_t traceTableEntry;

	std::array<bool, channels> previousEntry;

	RingBuffer<std::pair<std::array<bool, channels>, uint32_t>> traceTable{20000};
};
#endif