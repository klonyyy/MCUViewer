#include "JlinkHandler.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>
#include <windows.h>

#include <algorithm>
#include <string>

#include "logging.h"

JlinkHandler::JlinkHandler()
{
	int ret;
	so_handle = LoadLibrary("C:/Program Files/SEGGER/JLink/JLink_x64.dll");

	jlink_lock = (jlink_lock_t)GetProcAddress(so_handle, "JLock");
	jlink_open = (jlink_open_t)GetProcAddress(so_handle, "JLINKARM_OpenEx");
	jlink_is_open = (jlink_is_open_t)GetProcAddress(so_handle, "JLINKARM_IsOpen");
	jlink_read_mem = (jlink_read_mem_t)GetProcAddress(so_handle, "JLINKARM_ReadMemEx");

	ret = jlink_open(nullptr, nullptr);
	spdlog::info("Opening JLink {}", ret);

	ret = jlink_is_open();
	spdlog::info("Is open JLink {}", ret);
}

bool JlinkHandler::startAcqusition()
{
	bool ret = jlink_is_open();
	spdlog::info("Is open JLink {}", ret);
	return ret;
}
bool JlinkHandler::stopAcqusition()
{
	return true;
}
bool JlinkHandler::isValid() const
{
	return true;
}

bool JlinkHandler::readMemory(uint32_t address, uint32_t* value)
{
	uint8_t buf[4] = {0};
	int count;

	count = jlink_read_mem(address, 4, buf, nullptr);
	spdlog::debug("Read {} bytes", count);
	if (count <= 0)
		return false;

	spdlog::debug("Value {}", *(uint32_t*)buf);

	*value = *(uint32_t*)buf;
	return true;
}
bool JlinkHandler::writeMemory(uint32_t address, uint8_t* buf, uint32_t len)
{
	return false;
}

std::string JlinkHandler::getLastErrorMsg() const
{
	return "TODO";
}