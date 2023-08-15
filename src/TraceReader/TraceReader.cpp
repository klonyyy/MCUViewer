#include "TraceReader.hpp"

#include <cstring>
#include <random>

#define TRACE_OP_IS_OVERFLOW(c)		   (c == 0x70)
#define TRACE_OP_IS_LOCAL_TIME(c)	   ((c & 0x0f) == 0x00 && (c & 0x70) != 0x00)
#define TRACE_OP_IS_EXTENSION(c)	   ((c & 0x0b) == 0x08)
#define TRACE_OP_IS_GLOBAL_TIME(c)	   ((c & 0xdf) == 0x94)
#define TRACE_OP_IS_SOURCE(c)		   ((c & 0x03) != 0x00)
#define TRACE_OP_IS_SW_SOURCE(c)	   ((c & 0x03) != 0x00 && (c & 0x04) == 0x00)
#define TRACE_OP_IS_HW_SOURCE(c)	   ((c & 0x03) != 0x00 && (c & 0x04) == 0x04)
#define TRACE_OP_IS_TARGET_SOURCE(c)   (c & 0x01)
#define TRACE_OP_GET_CONTINUATION(c)   (c & 0x80)
#define TRACE_OP_GET_SOURCE_SIZE(c)	   (c & 0x03)
#define TRACE_OP_GET_SW_SOURCE_ADDR(c) (c >> 3)

#define TRACE_TIMEOUT_1 0xD0
#define TRACE_TIMEOUT_2 0xE0
#define TRACE_TIMEOUT_3 0xF0

TraceReader::TraceReader(std::shared_ptr<ITraceDevice> traceDevice, std::shared_ptr<spdlog::logger> logger) : traceDevice(traceDevice), logger(logger)
{
}

bool TraceReader::startAcqusition()
{
	for (auto& [key, value] : traceQuality)
		value = 0;

	if (traceDevice->startTrace(coreFrequency * 1000, traceFrequency))
	{
		isRunning = true;
		readerHandle = std::thread(&TraceReader::readerThread, this);
		return true;
	}
	traceDevice->stopTrace();
	return false;
}

bool TraceReader::stopAcqusition()
{
	isRunning = false;

	if (readerHandle.joinable())
		readerHandle.join();

	traceDevice->stopTrace();
	return true;
}

bool TraceReader::isValid() const
{
	return isRunning;
}

bool TraceReader::readTrace(double& timestamp, std::array<bool, 10>& trace)
{
	auto entry = traceTable.pop();
	timestamp = entry.second / static_cast<double>(coreFrequency);
	trace = entry.first;
	return true;
}

std::string TraceReader::getLastErrorMsg() const
{
	return lastErrorMsg;
}

void TraceReader::setCoreClockFrequency(uint32_t frequencyHz)
{
	coreFrequency = frequencyHz;
}

uint32_t TraceReader::getCoreClockFrequency() const
{
	return coreFrequency;
}

void TraceReader::setTraceFrequency(uint32_t frequencyHz)
{
	traceFrequency = frequencyHz;
}

uint32_t TraceReader::getTraceFrequency() const
{
	return traceFrequency;
}

std::map<const char*, uint32_t> TraceReader::getTraceIndicators() const
{
	return traceQuality;
}

TraceReader::TraceState TraceReader::updateTraceIdle(uint8_t c)
{
	if (TRACE_OP_IS_TARGET_SOURCE(c))
	{
		currentChannel[awaitingTimestamp] = (c & 0xf8) >> 3;
		return TRACE_STATE_TARGET_SOURCE;
	}
	else if (TRACE_OP_IS_LOCAL_TIME(c) || TRACE_OP_IS_GLOBAL_TIME(c))
	{
		memset(timestampBuf, 0, sizeof(timestampBuf));
		timestamp = 0;
		timestampBytes = 0;

		if (c == TRACE_TIMEOUT_1)
			traceQuality.at("delayed timestamp 1")++;
		else if (c == TRACE_TIMEOUT_2)
			traceQuality.at("delayed timestamp 2")++;
		else if (c == TRACE_TIMEOUT_3)
			traceQuality.at("delayed timestamp 3")++;

		if (TRACE_OP_GET_CONTINUATION(c))
			return TRACE_STATE_TARGET_TIMESTAMP_HEADER;
		else
		{
			timestampBuf[timestampBytes++] = c;
			timestampEnd();
			return TRACE_STATE_IDLE;
		}
	}
	else if (TRACE_OP_IS_EXTENSION(c))
		return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME : TRACE_STATE_IDLE;
	else if (TRACE_OP_IS_OVERFLOW(c))
		logger->warn("OVERFLOW OPTCODE 0x%02x\n", c);

	traceQuality["error frames"]++;

	return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME : TRACE_STATE_IDLE;
}

void TraceReader::timestampEnd()
{
	if (timestampBytes == 1)
		timestamp = (uint32_t)(timestampBuf[0] & 0x7f) >> 4;
	else
	{
		for (uint32_t i = 0; i < timestampBytes; i++)
			timestamp |= (uint32_t)(timestampBuf[i] & 0x7f) << 7 * i;
	}

	std::array<bool, channels> currentEntry{previousEntry};

	uint32_t i = 0;
	while (awaitingTimestamp--)
	{
		currentEntry[currentChannel[i]] = currentValue[i] == 0xaa ? true : false;
		i++;
	}

	traceTable.push(std::pair<std::array<bool, channels>, uint32_t>{currentEntry, timestamp});
	previousEntry = currentEntry;
	awaitingTimestamp = 0;
}

TraceReader::TraceState TraceReader::updateTrace(uint8_t c)
{
	if (state == TRACE_STATE_UNKNOWN)
	{
		logger->warn("STATE UNKNOWN {}", c);
		if (TRACE_OP_IS_TARGET_SOURCE(c) || TRACE_OP_IS_LOCAL_TIME(c) || TRACE_OP_IS_GLOBAL_TIME(c))
			state = TRACE_STATE_IDLE;
	}

	awaitingTimestamp = std::clamp(awaitingTimestamp, (uint8_t)0, (uint8_t)sizeof(currentValue));
	timestampBytes = std::clamp(timestampBytes, (uint32_t)0, (uint32_t)sizeof(timestampBuf));

	switch (state)
	{
		case TRACE_STATE_IDLE:
		{
			return updateTraceIdle(c);
		}

		case TRACE_STATE_TARGET_SOURCE:
		{
			currentValue[awaitingTimestamp++] = c;
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_TARGET_TIMESTAMP_HEADER:
		{
			timestampBuf[timestampBytes++] = c;
			if (TRACE_OP_GET_CONTINUATION(c))
				return TRACE_STATE_TARGET_TIMESTAMP_CONT;
			else
				timestampEnd();
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_TARGET_TIMESTAMP_CONT:
		{
			timestampBuf[timestampBytes++] = c;
			if (TRACE_OP_GET_CONTINUATION(c))
				return TRACE_STATE_TARGET_TIMESTAMP_CONT;
			else
				timestampEnd();
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_SKIP_FRAME:
			return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME
												: TRACE_STATE_IDLE;
		case TRACE_STATE_SKIP_4:
			return TRACE_STATE_SKIP_3;

		case TRACE_STATE_SKIP_3:
			return TRACE_STATE_SKIP_2;

		case TRACE_STATE_SKIP_2:
			return TRACE_STATE_SKIP_1;

		case TRACE_STATE_SKIP_1:
			return TRACE_STATE_IDLE;

		case TRACE_STATE_UNKNOWN:
			return TRACE_STATE_UNKNOWN;

		default:
			logger->error("Invalid state {}.  This should never happen", state);
			return TRACE_STATE_IDLE;
	}
}

void TraceReader::readerThread()
{
	while (isRunning)
	{
		int32_t length = traceDevice->readTraceBuffer(buffer, size);

		if (length < 0)
		{
			logger->error("CRITICAL ERROR");
			break;
		}

		if (length == 0)
		{
			logger->info("SLEEP");
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			continue;
		}

		if (length == size)
		{
			logger->error("OVERFLOW");
			continue;
		}

		for (int32_t i = 0; i < length; i++)
		{
			state = updateTrace(buffer[i]);
			if (!isRunning)
				break;
		}
	}
}