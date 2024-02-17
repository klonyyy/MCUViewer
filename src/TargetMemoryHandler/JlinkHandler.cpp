#include "JlinkHandler.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

JlinkHandler::JlinkHandler()
{
	isLoaded = dynamicLibraryLoader.doLoad(jlinkFunctions);

	if (!isLoaded)
		spdlog::error("Error loading jlink dynamic library!");
}

bool JlinkHandler::startAcqusition(const std::string& serialNumber)
{
	/* TODO */
	int serialNumberInt = std::atoi(serialNumber.c_str());

	// bool ret = jlinkFunctions.jlinkSelectByUsb(serialNumberInt);
	// if (!ret)
	// 	return false;

	jlinkFunctions.jlink_open(nullptr, nullptr);

	bool ret = jlinkFunctions.jlink_is_open();

	spdlog::info("Is open JLink {}", ret);
	isRunning = ret;
	return ret;
}
bool JlinkHandler::stopAcqusition()
{
	jlinkFunctions.jlink_close();
	return true;
}
bool JlinkHandler::isValid() const
{
	return isRunning;
}

bool JlinkHandler::readMemory(uint32_t address, uint32_t* value)
{
	if (!isRunning)
		return false;
	if (jlinkFunctions.jlink_read_mem(address, 4, (uint8_t*)value, 0) <= 0)
		return false;
	return true;
}
bool JlinkHandler::writeMemory(uint32_t address, uint8_t* buf, uint32_t len)
{
	if (!isRunning)
		return false;
	if (jlinkFunctions.jlink_write_mem(address, len, buf, 0) <= 0)
		return false;
	return true;
}

std::string JlinkHandler::getLastErrorMsg() const
{
	return lastErrorMsg;
}

std::vector<std::string> JlinkHandler::getConnectedDevices()
{
	JlinkFunctions::JLINKARM_EMU_CONNECT_INFO connectInfo[JlinkFunctions::maxDevices]{};
	int32_t result = jlinkFunctions.jlinkGetList(1, (JlinkFunctions::JLINKARM_EMU_CONNECT_INFO*)connectInfo, JlinkFunctions::maxDevices);

	if (result < 0)
	{
		spdlog::error("Error reading Jlink devices list. Error code {}", result);
		return std::vector<std::string>{};
	}

	std::vector<std::string> deviceIDs;

	for (size_t i = 0; i < JlinkFunctions::maxDevices; i++)
	{
		auto serialNumber = connectInfo[i].serialNumber;

		if (serialNumber != 0)
		{
			spdlog::info("Jlink serial number {}", serialNumber);
			deviceIDs.push_back(std::to_string(serialNumber));
		}
	}

	return deviceIDs;
}