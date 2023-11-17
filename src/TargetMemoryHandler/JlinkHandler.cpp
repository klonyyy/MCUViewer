#include "JlinkHandler.h"

#include <dlfcn.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>


JlinkHandler::JlinkHandler()
{
	int ret;
	so_handle = dlopen("/opt/SEGGER/JLink/libjlinkarm.so", RTLD_NOW);

	jlink_select_by_usb = (jlink_select_by_usb_t)dlsym(so_handle, "JLINKARM_EMU_SelectByUSBSN");
	jlink_lock = (jlink_lock_t)dlsym(so_handle, "JLock");
	jlink_open = (jlink_open_t)dlsym(so_handle, "JLINKARM_OpenEx");
	jlink_is_open = (jlink_is_open_t)dlsym(so_handle, "JLINKARM_IsOpen");
	jlink_read_mem = (jlink_read_mem_t)dlsym(so_handle, "JLINKARM_ReadMemEx");

	// Following connection path from function `open`: https://github.com/square/pylink/blob/master/pylink/jlink.py#L681k
	ret = jlink_select_by_usb(440237411);
	spdlog::info("Select result {}", ret);
	if (ret < 0)
	{
		return;
	}

	// Creates sig SEGV -> wrong parameter?
	//	ret = jlink_lock(440237411);
	//	spdlog::info("Loc result {}", ret);
	//	if (ret < 0) {
	//		return;
	//	}

	ret = jlink_open(nullptr, nullptr);
	spdlog::info("Opening JLink {}", ret);

	ret = jlink_is_open();
	spdlog::info("Is open JLink {}", ret);
}

bool JlinkHandler::startAcqusition()
{
	bool ret;
	ret = jlink_is_open();
	spdlog::info("Is open JLink {}", ret);

	return ret;
}
bool JlinkHandler::stopAcqusition()
{
	// TODO
	return true;
}
bool JlinkHandler::isValid() const
{
	// TODO
	return true;
}

bool JlinkHandler::readMemory(uint32_t address, uint32_t* value)
{
	uint8_t buf[4] = {0};
	int count;

	count = jlink_read_mem(address, 4, buf, nullptr);
	spdlog::debug("Read {} bytes", count);
	if (count <= 0)
	{
		return false;
	}

	spdlog::debug("Value {}", *(uint32_t * )buf);

	*value = *(uint32_t * )buf;
	return true;
}
bool JlinkHandler::writeMemory(uint32_t address, uint8_t* buf, uint32_t len)
{
	// TODO
	return false;
}

std::string JlinkHandler::getLastErrorMsg() const
{
	//	return lastErrorMsg;
	return "TODO";
}
