#include "MemoryReader.hpp"

bool MemoryReader::start(const IDebugProbe::DebugProbeSettings& probeSettings, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency) const
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->startAcqusition(probeSettings, addressSizeVector, samplingFreqency);
}
bool MemoryReader::stop() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->stopAcqusition();
}

std::optional<IDebugProbe::varEntryType> MemoryReader::readSingleEntry()
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->readSingleEntry();
}

uint32_t MemoryReader::getValue(uint32_t address, uint32_t size, bool& result)
{
	uint32_t value = 0;
	std::lock_guard<std::mutex> lock(mtx);

	result = probe->readMemory(address, &value);

	if (!result)
		return 0;

	if (probe->requiresAlignedAccessOnRead())
	{
		uint8_t shouldShift = address % 4;
		if (size == 1)
		{
			if (shouldShift == 0)
				value = (value & 0x000000ff);
			else if (shouldShift == 1)
				value = (value & 0x0000ff00) >> 8;
			else if (shouldShift == 2)
				value = (value & 0x00ff0000) >> 16;
			else if (shouldShift == 3)
				value = (value & 0xff000000) >> 24;
		}
		else if (size == 2)
		{
			if (shouldShift == 0)
				value = (value & 0x0000ffff);
			else if (shouldShift == 1)
				value = (value & 0x00ffff00) >> 8;
			else if (shouldShift == 2)
				value = (value & 0xffff0000) >> 16;
			else if (shouldShift == 3)
				value = (value & 0x000000ff) << 8 | (value & 0xff000000) >> 24;
		}
		else if (size == 4)
		{
			if (shouldShift == 1)
				value = (value & 0x000000ff) << 24 | (value & 0xffffff00) >> 8;
			else if (shouldShift == 2)
				value = (value & 0x0000ffff) << 16 | (value & 0xffff0000) >> 16;
			else if (shouldShift == 3)
				value = (value & 0x00ffffff) << 24 | (value & 0xff000000) >> 8;
		}
	}

	return value;
}

bool MemoryReader::setValue(uint32_t address, uint32_t size, uint8_t* buf)
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->writeMemory(address, buf, size);
}

std::string MemoryReader::getLastErrorMsg() const
{
	/* TODO lock with timeout as we dont really care if we get it every cycle */
	return probe->getLastErrorMsg();
}

std::vector<std::string> MemoryReader::getConnectedDevices() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->getConnectedDevices();
}

void MemoryReader::changeDevice(std::shared_ptr<IDebugProbe> newProbe)
{
	std::lock_guard<std::mutex> lock(mtx);
	probe = newProbe;
}

std::string MemoryReader::getTargetName()
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->getTargetName();
}