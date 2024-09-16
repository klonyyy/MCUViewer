#include "StlinkDebugProbe.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

#include "logging.h"

StlinkDebugProbe::StlinkDebugProbe(spdlog::logger* logger) : logger(logger)
{
	init_chipids(const_cast<char*>("./chips"));
}

bool StlinkDebugProbe::startAcqusition(const DebugProbeSettings& probeSettings, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency)
{
	sl = stlink_open_usb(UINFO, CONNECT_HOT_PLUG, (char*)probeSettings.serialNumber.data(), probeSettings.speedkHz);
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
bool StlinkDebugProbe::stopAcqusition()
{
	isRunning = false;
	stlink_close(sl);
	return true;
}
bool StlinkDebugProbe::isValid() const
{
	return isRunning;
}

std::optional<IDebugProbe::varEntryType> StlinkDebugProbe::readSingleEntry()
{
	return std::nullopt;
}

bool StlinkDebugProbe::readMemory(uint32_t address, uint32_t* value)
{
	if (!isRunning)
		return false;
	return stlink_read_debug32(sl, address, value) == 0;
}
bool StlinkDebugProbe::writeMemory(uint32_t address, uint8_t* buf, uint32_t len)
{
	if (!isRunning)
		return false;
	std::copy(buf, buf + len, sl->q_buf);
	return stlink_write_mem8(sl, address, len) == 0;
}

std::string StlinkDebugProbe::getLastErrorMsg() const
{
	return lastErrorMsg;
}

std::vector<std::string> StlinkDebugProbe::getConnectedDevices()
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
			logger->info("STLink serial number {}", serialNumber);
			deviceIDs.push_back(serialNumber);
		}
	}

	stlink_probe_usb_free(&stdevs, size);

	return deviceIDs;
}