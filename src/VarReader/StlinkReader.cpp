#include "StlinkReader.hpp"

bool StlinkReader::startAcqusition()
{
	sl = stlink_open_usb(UERROR, CONNECT_HOT_PLUG, NULL, 4000);
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
bool StlinkReader::stopAcqusition()
{
	isRunning = false;
	stlink_close(sl);
	return true;
}
bool StlinkReader::isValid() const
{
	return isRunning;
}

bool StlinkReader::readMemory(uint32_t address, uint32_t* value)
{
	if (!isRunning)
		return false;
	return stlink_read_debug32(sl, address, value) == 0;
}
bool StlinkReader::writeMemory(uint32_t address, uint8_t* buf, uint32_t len)
{
	if (!isRunning)
		return false;
	memcpy(sl->q_buf, buf, len);
	return stlink_write_mem8(sl, address, len) == 0;
}

std::string StlinkReader::getLastErrorMsg() const
{
	return lastErrorMsg;
}