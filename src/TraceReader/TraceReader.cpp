#include "TraceReader.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <random>
#include <utility>

#define TRACE_OP_IS_OVERFLOW(c)		   (c == 0x70)
#define TRACE_OP_IS_LOCAL_TIME(c)	   ((c & 0x0f) == 0x00 && (c & 0x70) != 0x00)
#define TRACE_OP_IS_SW_SOURCE(c)	   ((c & 0x03) != 0x00 && (c & 0x04) == 0x00)
#define TRACE_OP_GET_CONTINUATION(c)   (c & 0x80)
#define TRACE_OP_GET_SOURCE_SIZE(c)	   (c & 0x03)
#define TRACE_OP_GET_SW_SOURCE_ADDR(c) (c >> 3)
#define TRACE_TIMEOUT_1				   0xD0
#define TRACE_TIMEOUT_2				   0xE0
#define TRACE_TIMEOUT_3				   0xF0
#define TRACE_OP_IS_EXTENSION(c)	   ((c & 0x0b) == 0x08)

TraceReader::TraceReader(spdlog::logger* logger) : logger(logger)
{
}

bool TraceReader::startAcqusition(const ITraceProbe::TraceProbeSettings& probeSettings, const std::array<bool, 32>& activeChannels)
{
	traceIndicators = {};

	uint32_t activeChannelsMask = 0;
	for (uint32_t i = 0; i < activeChannels.size(); i++)
		activeChannelsMask |= (static_cast<uint32_t>(activeChannels[i]) << i);

	if (tracePrescaler == 0)
	{
		lastErrorMsg = "Trace prescaler cannot be zero!";
		return false;
	}
	else if (coreFrequency == 0)
	{
		lastErrorMsg = "Core frequency cannot be zero!";
		return false;
	}

	if (TraceProbe->startTrace(probeSettings, coreFrequency * 1000, tracePrescaler, activeChannelsMask, shouldReset))
	{
		lastErrorMsg = "";
		isRunning = true;
		readerHandle = std::thread(&TraceReader::readerThread, this);
		return true;
	}
	lastErrorMsg = "Trace probe not found!";
	stopAcqusition();
	return false;
}

bool TraceReader::stopAcqusition()
{
	isRunning = false;
	traceIndicators.sleepCycles = 0;

	if (readerHandle.joinable())
		readerHandle.join();

	return true;
}

bool TraceReader::isValid() const
{
	return isRunning;
}

bool TraceReader::readTrace(double& timestamp, std::array<uint32_t, 10>& trace)
{
	if (!isRunning || traceTable.size() == 0)
		return false;
	auto entry = traceTable.pop();
	timestamp = entry.second / static_cast<double>(coreFrequency * 1000);
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
	tracePrescaler = frequencyHz;
}

uint32_t TraceReader::getTraceFrequency() const
{
	return tracePrescaler;
}

void TraceReader::setTraceShouldReset(bool shouldReset)
{
	this->shouldReset = shouldReset;
}

void TraceReader::setTraceTimeout(uint32_t timeout)
{
	traceTimeout = timeout;
}

std::vector<std::string> TraceReader::getConnectedDevices() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return TraceProbe->getConnectedDevices();
}

void TraceReader::changeDevice(std::shared_ptr<ITraceProbe> newTraceProbe)
{
	std::lock_guard<std::mutex> lock(mtx);
	TraceProbe = newTraceProbe;
}

std::string TraceReader::getTargetName()
{
	std::lock_guard<std::mutex> lock(mtx);
	return TraceProbe->getTargetName();
}

TraceReader::TraceIndicators TraceReader::getTraceIndicators() const
{
	return traceIndicators;
}

TraceReader::TraceState TraceReader::updateTraceIdle(uint8_t c)
{
	traceIndicators.framesTotal++;

	if (TRACE_OP_IS_SW_SOURCE(c))
	{
		sourceFrameSize = (c & 0x03);
		currentChannel[awaitingTimestamp] = (c & 0xf8) >> 3;
		return TRACE_STATE_TARGET_SOURCE_1B;
	}
	else if (TRACE_OP_IS_LOCAL_TIME(c))
	{
		timestampVec.clear();
		timestamp = 0;

		if (c == TRACE_TIMEOUT_1)
			traceIndicators.delayedTimestamp1++;
		else if (c == TRACE_TIMEOUT_2)
			traceIndicators.delayedTimestamp2++;
		else if (c == TRACE_TIMEOUT_3)
			traceIndicators.delayedTimestamp3++;

		if (TRACE_OP_GET_CONTINUATION(c))
			return TRACE_STATE_TARGET_TIMESTAMP_HEADER;
		else
		{
			timestampVec.push_back(c);
			timestampEnd(true);
			return TRACE_STATE_IDLE;
		}
	}
	else if (TRACE_OP_IS_EXTENSION(c))
		return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME : TRACE_STATE_IDLE;
	else if (TRACE_OP_IS_OVERFLOW(c))
		logger->error("OVERFLOW OPTCODE {}", c);

	logger->debug("Unknown optcode {}", c);
	traceIndicators.errorFramesTotal++;

	return TRACE_STATE_IDLE;
}

void TraceReader::timestampEnd(bool headerData)
{
	if (headerData)
		timestamp = (uint32_t)(timestampVec[0] & 0x7f) >> 4;
	else
	{
		for (uint32_t i = 0; i < timestampVec.size(); i++)
			timestamp |= (uint32_t)(timestampVec[i] & 0x7f) << 7 * i;
	}

	std::array<uint32_t, channels> currentEntry{previousEntry};

	uint32_t i = 0;
	while (awaitingTimestamp--)
	{
		if (currentChannel[i] > channels || i >= channels - 1)
		{
			traceIndicators.errorFramesTotal++;
			logger->debug("Wrong channel id {}, {}", i, currentChannel[i]);
			break;
		}
		currentEntry[currentChannel[i]] = currentValue[i];
		i++;
	}

	traceTable.push(std::pair<std::array<uint32_t, channels>, double>{currentEntry, timestamp});
	previousEntry = currentEntry;
	awaitingTimestamp = 0;
}

TraceReader::TraceState TraceReader::updateTrace(uint8_t c)
{
	awaitingTimestamp = std::clamp(awaitingTimestamp, (uint8_t)0, (uint8_t)(sizeof(currentValue) / sizeof(currentValue[0])));

	switch (state)
	{
		case TRACE_STATE_IDLE:
			return updateTraceIdle(c);

		case TRACE_STATE_TARGET_SOURCE_1B:
		{
			currentValue[awaitingTimestamp] = c;
			if (sourceFrameSize == 0x01)
			{
				awaitingTimestamp++;
				return TRACE_STATE_IDLE;
			}
			return TRACE_STATE_TARGET_SOURCE_2B;
		}
		case TRACE_STATE_TARGET_SOURCE_2B:
		{
			currentValue[awaitingTimestamp] |= (c << 8);
			if (sourceFrameSize == 0x02)
			{
				awaitingTimestamp++;
				return TRACE_STATE_IDLE;
			}
			return TRACE_STATE_TARGET_SOURCE_3B;
		}
		case TRACE_STATE_TARGET_SOURCE_3B:
		{
			currentValue[awaitingTimestamp] |= (c << 16);
			return TRACE_STATE_TARGET_SOURCE_4B;
		}
		case TRACE_STATE_TARGET_SOURCE_4B:
		{
			currentValue[awaitingTimestamp++] |= (c << 24);
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_TARGET_TIMESTAMP_HEADER:
		{
			timestampVec.push_back(c);
			if (TRACE_OP_GET_CONTINUATION(c))
				return TRACE_STATE_TARGET_TIMESTAMP_CONT;
			else
				timestampEnd(false);
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_TARGET_TIMESTAMP_CONT:
		{
			timestampVec.push_back(c);
			if (TRACE_OP_GET_CONTINUATION(c))
				return TRACE_STATE_TARGET_TIMESTAMP_CONT;
			else
				timestampEnd(false);
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_SKIP_FRAME:
			return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME
												: TRACE_STATE_IDLE;

		default:
			logger->critical("Invalid state! {}", static_cast<int>(state));
			return TRACE_STATE_IDLE;
	}
}

void TraceReader::readerThread()
{
	while (isRunning)
	{
		int32_t length = TraceProbe->readTraceBuffer(buffer, size);

		if (length < 0)
		{
			lastErrorMsg = "Trace probe critical error!";
			logger->error(lastErrorMsg);

			isRunning = false;
			break;
		}

		if (traceTimeout != 0 && traceIndicators.sleepCycles > traceTimeout * 10000)
		{
			lastErrorMsg = "No trace registered for " + std::to_string(traceTimeout) + "s!";
			logger->error(lastErrorMsg);
			isRunning = false;
			break;
		}

		if (length == 0)
		{
			traceIndicators.sleepCycles++;
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			continue;
		}

		traceIndicators.sleepCycles = 0;

		for (int32_t i = 0; i < length; i++)
		{
			state = updateTrace(buffer[i]);
			if (!isRunning)
				break;
		}
	}
	TraceProbe->stopTrace();
	logger->info("Closing trace thread {}", static_cast<bool>(isRunning));
}