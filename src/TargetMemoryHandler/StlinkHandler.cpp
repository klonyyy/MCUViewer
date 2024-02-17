#include "StlinkHandler.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

#include "logging.h"

StlinkHandler::StlinkHandler()
{
	init_chipids(const_cast<char*>("./chips"));
}

bool StlinkHandler::startAcqusition(const std::string& serialNumber, const std::string& device)
{
	sl = stlink_open_usb(UINFO, CONNECT_HOT_PLUG, (char*)serialNumber.data(), 24000);
	isRunning = false;

	if (sl != nullptr)
	{
		if (stlink_enter_swd_mode(sl) != 0 || stlink_target_connect(sl, CONNECT_HOT_PLUG) != 0)
		{
			stopAcqusition();
			lastErrorMsg = "STM32 target not found!";
			return false;
		}

		isRunning = true;
		lastErrorMsg = "";
		return true;
	}
	lastErrorMsg = "STLink not found!";
	return false;
}
bool StlinkHandler::stopAcqusition()
{
	isRunning = false;
	stlink_close(sl);
	return true;
}
bool StlinkHandler::isValid() const
{
	return isRunning;
}

bool StlinkHandler::readMemory(uint32_t address, uint32_t* value)
{
	if (!isRunning)
		return false;
	return stlink_read_debug32(sl, address, value) == 0;
}
bool StlinkHandler::writeMemory(uint32_t address, uint8_t* buf, uint32_t len)
{
	if (!isRunning)
		return false;
	std::copy(buf, buf + len, sl->q_buf);
	return stlink_write_mem8(sl, address, len) == 0;
}

std::string StlinkHandler::getLastErrorMsg() const
{
	return lastErrorMsg;
}

std::vector<std::string> StlinkHandler::getConnectedDevices()
{
	stlink_t** stdevs;
	uint32_t size;

	size = stlink_probe_usb(&stdevs, CONNECT_HOT_PLUG, 24000);

	std::vector<std::string> deviceIDs;

	for (size_t i = 0; i < size; i++)
	{
		std::string serialNumber{stdevs[i]->serial};

		if (!serialNumber.empty())
		{
			/* TODO */
			spdlog::info("serial {}", serialNumber);
			deviceIDs.push_back(serialNumber);
		}
	}

	stlink_probe_usb_free(&stdevs, size);

	return deviceIDs;
}