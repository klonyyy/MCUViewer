#include "VarReader.hpp"

#include <map>

#include "iostream"

VarReader::VarReader()
{
}
VarReader::~VarReader()
{
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

float VarReader::getFloat(uint32_t address, Variable::type type)
{
	uint32_t value = 0;
	if (sl != nullptr)
		stlink_read_debug32(sl, address, (uint32_t*)&value);

	if (type == Variable::type::U8)
		return (float)*(uint8_t*)&value;
	else if (type == Variable::type::I8)
		return (float)*(int8_t*)&value;
	else if (type == Variable::type::U16)
		return (float)*(uint16_t*)&value;
	else if (type == Variable::type::I16)
		return (float)*(int16_t*)&value;
	else if (type == Variable::type::U32)
		return (float)*(uint32_t*)&value;
	else if (type == Variable::type::I32)
		return (float)*(int32_t*)&value;
	else if (type == Variable::type::F32)
		return *(float*)&value;

	return 0.0f;
}