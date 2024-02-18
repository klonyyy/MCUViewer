#include "JlinkHandler.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

JlinkHandler::JlinkHandler(spdlog::logger* logger) : logger(logger)
{
	isLoaded = dynamicLibraryLoader.doLoad(jlinkFunctions);

	if (!isLoaded)
	{
		lastErrorMsg = "Could not load the JLink library!";
		logger->error(lastErrorMsg);
	}
}

bool JlinkHandler::startAcqusition(const std::string& serialNumber, const std::string& device)
{
	if (!isLoaded)
		return false;

	int serialNumberInt = std::atoi(serialNumber.c_str());

	if (jlinkFunctions.jlinkSelectByUsb(serialNumberInt) < 0)
	{
		lastErrorMsg = "Could not connect to the selected probe";
		return false;
	}
	lastErrorMsg = "";

	auto deviceCmd = "Device = " + device;
	jlinkFunctions.jlinkExecCommand(deviceCmd.c_str(), nullptr, 0);

	if (jlinkFunctions.jlinkOpen(nullptr, nullptr) != nullptr)
	{
		isRunning = false;
		return false;
	}

	isRunning = jlinkFunctions.jlinkIsOpen();

	return isRunning;
}
bool JlinkHandler::stopAcqusition()
{
	if (!isLoaded)
		return false;

	jlinkFunctions.jlinkClose();
	return true;
}
bool JlinkHandler::isValid() const
{
	return isRunning;
}

bool JlinkHandler::readMemory(uint32_t address, uint32_t* value)
{
	return (isLoaded && isRunning && jlinkFunctions.jlinkReadMem(address, 4, (uint8_t*)value, 0) >= 0);
}
bool JlinkHandler::writeMemory(uint32_t address, uint8_t* buf, uint32_t len)
{
	return (isLoaded && isRunning && jlinkFunctions.jlinkWriteMem(address, len, buf, 0) >= 0);
}

std::string JlinkHandler::getLastErrorMsg() const
{
	return lastErrorMsg;
}

std::vector<std::string> JlinkHandler::getConnectedDevices()
{
	std::vector<std::string> deviceIDs{};

	if (!isLoaded)
		return deviceIDs;

	JlinkFunctions::JLINKARM_EMU_CONNECT_INFO connectInfo[JlinkFunctions::maxDevices]{};
	int32_t result = jlinkFunctions.jlinkGetList(1, (JlinkFunctions::JLINKARM_EMU_CONNECT_INFO*)connectInfo, JlinkFunctions::maxDevices);

	if (result < 0)
	{
		logger->error("Error reading JLink devices list. Error code {}", result);
		return std::vector<std::string>{};
	}

	for (size_t i = 0; i < JlinkFunctions::maxDevices; i++)
	{
		auto serialNumber = connectInfo[i].serialNumber;

		if (serialNumber != 0)
		{
			logger->info("JLink serial number {}", serialNumber);
			deviceIDs.push_back(std::to_string(serialNumber));
		}
	}

	return deviceIDs;
}