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
	volatile uint32_t value = 0;

	uint8_t shouldShift = 0;

	if (address % 4 != 0)
	{
		shouldShift = address % 4;
		address = (address / 4) * 4;
	}

	if (sl != nullptr)
		stlink_read_debug32(sl, address, (uint32_t*)&value);

	if (shouldShift && (type == Variable::type::I8 || type == Variable::type::U8))
	{
		if (shouldShift == 1)
			value = (value & 0x0000ff00) >> 8;
		else if (shouldShift == 2)
			value = (value & 0x00ff0000) >> 16;
		else if (shouldShift == 3)
			value = (value & 0xff000000) >> 24;
	}
	else if (shouldShift && (type == Variable::type::I16 || type == Variable::type::U16))
	{
		if (shouldShift == 1)
			value = (value & 0x00ffff00) >> 8;
		else if (shouldShift == 2)
			value = (value & 0xffff0000) >> 16;
	}

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