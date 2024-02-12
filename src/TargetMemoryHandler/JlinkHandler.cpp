#include "JlinkHandler.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>
#include <windows.h>

#include <algorithm>
#include <string>

#include "logging.h"

JlinkHandler::JlinkHandler()
{
	so_handle = LoadLibrary("C:/Program Files/SEGGER/JLink/JLink_x64.dll");
	jlinkFunctions.jlink_lock = (decltype(jlinkFunctions.jlink_lock))GetProcAddress(so_handle, "JLock");
	jlinkFunctions.jlink_open = (decltype(jlinkFunctions.jlink_open))GetProcAddress(so_handle, "JLINKARM_OpenEx");
	jlinkFunctions.jlink_close = (decltype(jlinkFunctions.jlink_close))GetProcAddress(so_handle, "JLINKARM_Close");
	jlinkFunctions.jlink_is_open = (decltype(jlinkFunctions.jlink_is_open))GetProcAddress(so_handle, "JLINKARM_IsOpen");
	jlinkFunctions.jlink_read_mem = (decltype(jlinkFunctions.jlink_read_mem))GetProcAddress(so_handle, "JLINKARM_ReadMemEx");
	jlinkFunctions.jlink_write_mem = (decltype(jlinkFunctions.jlink_write_mem))GetProcAddress(so_handle, "JLINKARM_WriteMemEx");
	jlinkFunctions.jlinkGetList = (decltype(jlinkFunctions.jlinkGetList))GetProcAddress(so_handle, "JLINKARM_EMU_GetList");
}

JlinkHandler::~JlinkHandler()
{
	FreeLibrary(so_handle);
}

bool JlinkHandler::startAcqusition()
{
	bool ret = jlinkFunctions.jlink_open(nullptr, nullptr);
	ret = jlinkFunctions.jlink_is_open();
	spdlog::info("Is open JLink {}", ret);
	isRunning = true;
	int a = sizeof(JlinkFunctions::JLINKARM_EMU_CONNECT_INFO);
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

std::vector<uint32_t> JlinkHandler::getConnectedDevices()
{
	JlinkFunctions::JLINKARM_EMU_CONNECT_INFO connectInfo[JlinkFunctions::maxDevices]{};
	int32_t result = jlinkFunctions.jlinkGetList(1, (JlinkFunctions::JLINKARM_EMU_CONNECT_INFO*)connectInfo, JlinkFunctions::maxDevices);

	if (result < 0)
	{
		spdlog::error("Error reading Jlink devices list. Error code {}", result);
		return std::vector<uint32_t>{};
	}

	std::vector<uint32_t> deviceIDs;

	for (size_t i = 0; i < JlinkFunctions::maxDevices; i++)
	{
		auto serialNumber = connectInfo[i].serialNumber;

		if (serialNumber != 0)
		{
			spdlog::info("Jlink serial number {}", serialNumber);
			deviceIDs.push_back(serialNumber);
		}
	}

	return deviceIDs;
}