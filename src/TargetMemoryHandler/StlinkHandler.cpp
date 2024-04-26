#include "StlinkHandler.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

#include "logging.h"
#include "register.h"

StlinkHandler::StlinkHandler(spdlog::logger* logger) : logger(logger)
{
	init_chipids(const_cast<char*>("./chips"));
}

bool StlinkHandler::startAcqusition(const std::string& serialNumber, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency, Mode mode, const std::string& device)
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

std::optional<IDebugProbe::varEntryType> StlinkHandler::readSingleEntry()
{
	return std::nullopt;
}

int StlinkHandler::IsChipHalted(stlink_t* sl)
{
	uint32_t dhcsr = 0;
	bool isError = stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr) != 0;

	if (isError) {
		logger->error("Error while reading DHCSR register");
		return -1;
	} else {
		if ((dhcsr & STLINK_REG_DHCSR_S_HALT) != 0) {
			return 1;
		} else {
			return 0;
		}
	}
}

int StlinkHandler::IsChipSleeping(stlink_t* sl)
{
	uint32_t dhcsr = 0;
	bool isError = stlink_read_debug32(sl, STLINK_REG_DHCSR, &dhcsr) != 0;

	if (isError) {
		logger->error("Error while reading DHCSR register");
		return -1;
	} else {
		if ((dhcsr & STLINK_REG_DHCSR_S_SLEEP) != 0) {
			return 1;
		} else {
			return 0;
		}
	}
}

bool StlinkHandler::IsWaitForWakeSuccess(stlink_t* sl)
{
	int timeout = 1000;
	int sleep = -1;
	int halt = -1;

	while (timeout-- > 0) {
		sleep = IsChipSleeping(sl);
		halt = IsChipHalted(sl);

		if (sleep == 0 && halt == 1) {
			return true;
		}
	}
	return false;
}

bool StlinkHandler::IsWaitForResumeSuccess(stlink_t* sl)
{
	int timeout = 1000;
	int halt = -1;

	while (timeout-- > 0) {
		halt = IsChipHalted(sl);

		if (halt == 0) {
			return true;
		}
	}
	return false;
}

bool StlinkHandler::readMemory(uint32_t address, uint32_t* value)
{
	if (!isRunning)
		return false;

	bool result = true;

	// Check if the chip is sleeping
	int wasSleeping = IsChipSleeping(sl);
	if (wasSleeping == 0)
		// Try to read the memory
		result &= stlink_read_debug32(sl, address, value) == 0;
	else if (wasSleeping == -1)
		return false;
	else
		*value = 0;	// In case when read in sleep not allowed, return 0 when chip is sleeping

	if (isReadWhileSleepAllowed) {
		// If the chip entered sleep during read operation, we need to wake it up and retry read
		int isSleeping = IsChipSleeping(sl);
		if (isSleeping == 1 || wasSleeping == 1) {
			// Halt the chip to wake it up
			result &= stlink_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN | STLINK_REG_DHCSR_C_HALT) == 0;
			// Wait for the chip to halt and wake up
			result &= IsWaitForWakeSuccess(sl);
			// Retry read
			result &= stlink_read_debug32(sl, address, value) == 0;
			// Resume the chip
			result &= stlink_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN) == 0;
			// Wait for the chip to resume
			result &= IsWaitForResumeSuccess(sl);
			// TODO: Find a way to return chip into sleep mode (and make sure that no interrupts are generated during this read operation)
		} else if (isSleeping == -1) {
			return false;
		}
	}
	
	return result;
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
			logger->info("STLink serial number {}", serialNumber);
			deviceIDs.push_back(serialNumber);
		}
	}

	stlink_probe_usb_free(&stdevs, size);

	return deviceIDs;
}