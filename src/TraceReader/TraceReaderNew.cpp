#include "TraceReaderNew.hpp"

#include <cstring>

#define TRACE_OP_IS_OVERFLOW(c)			(c == 0x70)
#define TRACE_OP_IS_LOCAL_TIME(c)		((c & 0x0f) == 0x00 && (c & 0x70) != 0x00)
#define TRACE_OP_IS_EXTENSION(c)		((c & 0x0b) == 0x08)
#define TRACE_OP_IS_GLOBAL_TIME(c)		((c & 0xdf) == 0x94)
#define TRACE_OP_IS_SOURCE(c)			((c & 0x03) != 0x00)
#define TRACE_OP_IS_SW_SOURCE(c)		((c & 0x03) != 0x00 && (c & 0x04) == 0x00)
#define TRACE_OP_IS_HW_SOURCE(c)		((c & 0x03) != 0x00 && (c & 0x04) == 0x04)
#define TRACE_OP_IS_TARGET_SOURCE_1B(c) ((c & 0x03) == 0x01)
#define TRACE_OP_IS_TARGET_SOURCE_4B(c) ((c & 0x03) == 0x03)
#define TRACE_OP_GET_CONTINUATION(c)	(c & 0x80)
#define TRACE_OP_GET_SOURCE_SIZE(c)		(c & 0x03)
#define TRACE_OP_GET_SW_SOURCE_ADDR(c)	(c >> 3)

#define TRACE_TIMEOUT_1 0xD0
#define TRACE_TIMEOUT_2 0xE0
#define TRACE_TIMEOUT_3 0xF0

TraceReaderNew::TraceReaderNew(std::shared_ptr<ITraceDevice> traceDevice, std::shared_ptr<spdlog::logger> logger) : traceDevice(traceDevice), logger(logger)
{
	traceTable = std::make_unique<RingBuffer<std::pair<std::array<uint32_t, channels>, double>>>(20000);
}

bool TraceReaderNew::startAcqusition(std::array<bool, 32>& activeChannels)
{
	for (auto& [key, value] : traceQuality)
		value = 0;

	uint32_t activeChannelsMask = 0;
	for (uint32_t i = 0; i < activeChannels.size(); i++)
		activeChannelsMask |= (static_cast<uint32_t>(activeChannels[i]) << i);

	if (traceDevice->startTrace(coreFrequency * 1000, tracePrescaler, activeChannelsMask))
	{
		lastErrorMsg = "";
		isRunning = true;
		readerHandle = std::thread(&TraceReaderNew::readerThread, this);
		return true;
	}
	traceDevice->stopTrace();
	return false;
}

bool TraceReaderNew::stopAcqusition()
{
	isRunning = false;
	traceQuality["sleep cycles"] = 0;

	if (readerHandle.joinable())
		readerHandle.join();

	traceDevice->stopTrace();
	return true;
}

bool TraceReaderNew::isValid() const
{
	return isRunning.load();
}

bool TraceReaderNew::readTrace(double& timestamp, std::array<uint32_t, 10>& trace)
{
	if (!isRunning.load() || traceTable->getSize() == 0)
		return false;
	auto entry = traceTable->pop();
	timestamp = entry.second / static_cast<double>(coreFrequency * 1000);
	trace = entry.first;
	return true;
}

std::string TraceReaderNew::getLastErrorMsg() const
{
	return lastErrorMsg;
}

void TraceReaderNew::setCoreClockFrequency(uint32_t frequencyHz)
{
	coreFrequency = frequencyHz;
}

uint32_t TraceReaderNew::getCoreClockFrequency() const
{
	return coreFrequency;
}

void TraceReaderNew::setTraceFrequency(uint32_t frequencyHz)
{
	tracePrescaler = frequencyHz;
}

uint32_t TraceReaderNew::getTraceFrequency() const
{
	return tracePrescaler;
}

std::map<const char*, uint32_t> TraceReaderNew::getTraceIndicators() const
{
	return traceQuality;
}

void TraceReaderNew::processSource(std::vector<uint8_t>& chunk)
{
	std::cout << "source chunk:";
	for (auto& c : chunk)
	{
		std::cout << std::hex << (int)c << " ";
	}
	std::cout << std::endl;

	currentChannel[awaitingTimestamp] = (chunk[0] & 0xf8) >> 3;

	std::cout << "current channel " << (int)currentChannel[awaitingTimestamp] << "    " << (int)awaitingTimestamp << std::endl;

	for (uint32_t i = 1; i < chunk.size(); i++)
		currentValue[awaitingTimestamp] |= chunk[i] << 8 * (i - 1);

	awaitingTimestamp++;
	chunk.clear();
}

void TraceReaderNew::processTimestamp(std::vector<uint8_t>& chunk)
{
	std::cout << "timestamp chunk:";
	for (auto& c : chunk)
	{
		std::cout << std::hex << (int)c << " ";
	}
	std::cout << std::endl;

	if (chunk.size() == 1)
		timestamp = (uint32_t)(chunk[0] & 0x7f) >> 4;
	else
	{
		for (uint32_t i = 1; i < chunk.size(); i++)
			timestamp |= (uint32_t)(chunk[i] & 0x7f) << 7 * (i - 1);
	}

	std::cout << "timestamp double: " << (double)timestamp / 160000000.0 << std::endl;

	std::array<uint32_t, channels>
		currentEntry{previousEntry};

	uint32_t i = 0;
	while (awaitingTimestamp--)
	{
		if (currentChannel[i] > channels || i > channels)
		{
			traceQuality["error frames"]++;
			logger->error("WRONG CHANNEL {}", currentChannel[i]);
			break;
		}
		currentEntry[currentChannel[i]] = currentValue[i];
		std::cout << "current channel " << (int)currentChannel[i] << "   value  " << (int)currentValue[i] << std::endl;
		i++;
	}

	traceTable->push(std::pair<std::array<uint32_t, channels>, double>{currentEntry, timestamp});
	previousEntry = currentEntry;
	awaitingTimestamp = 0;
	std::memset(currentValue, 0, sizeof(currentValue));
	chunk.clear();
}

void TraceReaderNew::readerThread()
{
	while (isRunning.load())
	{
		int32_t length = traceDevice->readTraceBuffer(buffer, size);

		if (length < 0)
		{
			lastErrorMsg = "Stlink trace critical error!";
			logger->error(lastErrorMsg);

			isRunning.store(false);
			break;
		}

		if (traceQuality["sleep cycles"] > 20000)
		{
			lastErrorMsg = "No trace registered for 2s!";
			logger->error(lastErrorMsg);
			isRunning.store(false);
			break;
		}

		if (length == 0)
		{
			traceQuality["sleep cycles"]++;
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			continue;
		}

		if (length == size)
		{
			lastErrorMsg = "Trace overflow!";
			logger->error(lastErrorMsg);
			isRunning.store(false);
			continue;
		}

		traceQuality["sleep cycles"] = 0;

		uint32_t idx = 0;

		if (remainingFrameType == 2)
		{
			while (TRACE_OP_GET_CONTINUATION(buffer[idx]))
			{
				chunk.push_back(buffer[idx]);
				if (idx < length - 1)
					idx++;
				else
				{
					std::cout << "    remaining timestamp REST   " << std::endl;
					remainingFrameType = 2;
					remainingBytes = 1;
				}
			}
			if (remainingFrameType == 0)
				processTimestamp(chunk);
		}
		else if (remainingFrameType == 1 && remainingBytes)
		{
			while (remainingBytes--)
			{
				chunk.push_back(buffer[idx]);
				if (idx < length - 1)
					idx++;
				else
					remainingFrameType = 1;
			}
			length -= idx;
			if (remainingFrameType == 0)
				processSource(chunk);
		}

		remainingFrameType = 0;

		while (idx < length)
		{
			uint8_t c = buffer[idx];

			std::cout << std::hex << (int)c << std::endl;

			if (TRACE_OP_IS_SOURCE(c))
			{
				uint8_t size = (c & 0x03);

				chunk.push_back(buffer[idx]);

				for (uint32_t i = 0; i < size; i++)
				{
					if (idx < length - 1)
						idx++;
					else
					{
						remainingFrameType = 1;
						remainingBytes = size - i;
						std::cout << "remaining: " << (int)remainingBytes << std::endl;
					}

					chunk.push_back(buffer[idx]);
				}
				if (remainingFrameType == 0)
					processSource(chunk);
			}
			else if (TRACE_OP_IS_LOCAL_TIME(c))
			{
				timestamp = 0;

				chunk.push_back(buffer[idx]);

				while (TRACE_OP_GET_CONTINUATION(buffer[idx]) && !remainingBytes)
				{
					if (idx < length - 1)
						idx++;
					else
					{
						std::cout << "    remaining timestamp    " << std::endl;
						remainingFrameType = 2;
						remainingBytes = 1;
					}
					chunk.push_back(buffer[idx]);
				}
				if (remainingFrameType == 0)
					processTimestamp(chunk);
			}

			idx++;

			if (!isRunning)
				break;
		}

		std::cout << "    NEXT BUFFER    " << std::endl;
	}
	logger->info("Closing trace thread {}", isRunning);
}