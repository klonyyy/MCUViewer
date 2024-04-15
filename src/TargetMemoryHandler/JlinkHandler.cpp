#include "JlinkHandler.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

JlinkHandler::JlinkHandler(spdlog::logger* logger) : logger(logger)
{
	varTable = std::make_unique<RingBuffer<varEntryType>>(2000);
}

bool JlinkHandler::startAcqusition(const std::string& serialNumber, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency, Mode mode, const std::string& device)
{
	int serialNumberInt = std::atoi(serialNumber.c_str());

	if (JLINKARM_EMU_SelectByUSBSN(serialNumberInt) < 0)
	{
		lastErrorMsg = "Could not connect to the selected probe";
		return false;
	}
	lastErrorMsg = "";

	auto deviceCmd = "Device = " + device;
	JLINKARM_ExecCommand(deviceCmd.c_str(), nullptr, 0);

	JLINKARM_SetSpeed(50000);

	/* TODO temporary: select interface */
	JLINKARM_TIF_Select(1);

	trackedVarsCount = 0;
	trackedVarsTotalSize = 4;
	for (auto [address, size] : addressSizeVector)
	{
		auto& desc = variableDesc[trackedVarsCount++];
		desc.Addr = address;
		desc.NumBytes = size;
		desc.Flags = 0;
		desc.Dummy = 0;
		addressSizeMap[address] = size;
		trackedVarsTotalSize += size;
	}

	/* TODO check for zero */
	uint32_t samplePeriodUs = 1000000 / samplingFreqency;

	if (JLINK_HSS_Start(variableDesc, trackedVarsCount, samplePeriodUs, JLINK_HSS_FLAG_TIMESTAMP_US) >= 0)
		isRunning = true;
	else
		isRunning = false;

	return isRunning;
}

bool JlinkHandler::stopAcqusition()
{
	JLINKARM_Close();
	return true;
}

bool JlinkHandler::isValid() const
{
	return isRunning;
}

std::optional<IDebugProbe::varEntryType> JlinkHandler::readSingleEntry()
{
	uint8_t rawBuffer[128]{};

	int32_t readSize = JLINK_HSS_Read(rawBuffer, sizeof(rawBuffer));

	for (size_t i = 0; i < readSize; i += trackedVarsTotalSize)	 // +1 is for the timestamp
	{
		varEntryType entry{};

		/* timestamp */
		entry.first = (*(uint32_t*)&rawBuffer[i]) / 1000000.0;

		int32_t k = i + 4;
		for (size_t j = 0; j < trackedVarsCount; j++)
		{
			uint32_t address = variableDesc[j].Addr;
			entry.second[address] = *(uint32_t*)&rawBuffer[k];
			k += addressSizeMap[address];
		}

		varTable->push(entry);
	}

	if (readSize < 0 || varTable->size() == 0)
		return std::nullopt;

	return varTable->pop();
}

bool JlinkHandler::readMemory(uint32_t address, uint32_t* value)
{
	return (isRunning && JLINKARM_ReadMemEx(address, 4, (uint8_t*)value, 0) >= 0);
}

bool JlinkHandler::writeMemory(uint32_t address, uint8_t* buf, uint32_t len)
{
	return (isRunning && JLINKARM_WriteMemEx(address, len, buf, 0) >= 0);
}

std::string JlinkHandler::getLastErrorMsg() const
{
	return lastErrorMsg;
}

std::vector<std::string> JlinkHandler::getConnectedDevices()
{
	std::vector<std::string> deviceIDs{};

	JLINKARM_EMU_CONNECT_INFO connectInfo[maxDevices]{};
	int32_t result = JLINKARM_EMU_GetList(1, (JLINKARM_EMU_CONNECT_INFO*)connectInfo, maxDevices);

	if (result < 0)
	{
		logger->error("Error reading JLink devices list. Error code {}", result);
		return std::vector<std::string>{};
	}

	for (size_t i = 0; i < maxDevices; i++)
	{
		auto serialNumber = connectInfo[i].SerialNumber;

		if (serialNumber != 0)
		{
			logger->info("JLink serial number {}", serialNumber);
			deviceIDs.push_back(std::to_string(serialNumber));
		}
	}

	return deviceIDs;
}