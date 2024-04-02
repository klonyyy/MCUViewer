#include "JlinkHandler.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

JlinkHandler::JlinkHandler(spdlog::logger* logger) : logger(logger)
{
}

bool JlinkHandler::startAcqusition(const std::string& serialNumber, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, Mode mode, const std::string& device)
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

	if (JLINKARM_OpenEx(nullptr, nullptr) != nullptr)
	{
		isRunning = false;
		return false;
	}

	isRunning = JLINKARM_IsOpen();
	/* TODO temporary: select interface */
	JLINKARM_TIF_Select(1);

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

bool JlinkHandler::initRead() const
{
	// initialize Jlink HSS buffer read

	return true;
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