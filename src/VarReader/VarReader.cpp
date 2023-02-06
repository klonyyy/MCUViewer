#include "VarReader.hpp"

#include "iostream"

VarReader::VarReader()
{
}
VarReader::~VarReader()
{
}

bool VarReader::start()
{
	if (readerState == state::RUN)
		return false;

	sl = stlink_open_usb((ugly_loglevel)10, (connect_type)0, NULL, 4000);

	if (sl != NULL)
	{
		std::cout << "STlink detected!" << std::endl;
		stlink_version(sl);
		stlink_enter_swd_mode(sl);
		readerState = state::RUN;
		threadHandle = std::thread(&VarReader::readerHandler, this);
		return true;
	}

	std::cout << "STLink not detected!" << std::endl;
	return false;
}
bool VarReader::stop()
{
	if (readerState == state::STOP)
		return false;

	readerState = state::STOP;
	threadHandle.join();
	stlink_exit_debug_mode(sl);
	stlink_close(sl);
	return true;
}
void VarReader::readerHandler()
{
	while (readerState == state::RUN)
	{
		stlink_read_debug32(sl, 0x20000038, (uint32_t*)&a);
	}
}
float VarReader::geta()
{
	return a;
}