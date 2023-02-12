#include "VarReader.hpp"

#include <map>

#include "iostream"

VarReader::VarReader()
{
}
VarReader::~VarReader()
{
	stlink_close(sl);
}

bool VarReader::start()
{
	sl = stlink_open_usb((ugly_loglevel)10, (connect_type)0, NULL, 4000);

	std::cout << "Startig! **************" << std::endl;

	if (sl != NULL)
	{
		std::cout << "STlink detected!" << std::endl;
		stlink_version(sl);
		stlink_enter_swd_mode(sl);

		return true;
	}

	std::cout << "STLink not detected!" << std::endl;
	return false;
}
bool VarReader::stop()
{
	readerState = state::STOP;
	stlink_close(sl);
	return true;
}

uint32_t VarReader::getValue(uint32_t address)
{
	return adrMap[address];
}

float VarReader::getFloat(uint32_t address)
{
	uint32_t value = 0;
	if (sl != nullptr)
		stlink_read_debug32(sl, address, (uint32_t*)&value);
	return *(float*)&value;
}