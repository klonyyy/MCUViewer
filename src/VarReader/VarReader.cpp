#include "VarReader.hpp"

#include <map>

#include "iostream"

bool VarReader::start()
{
	sl = stlink_open_usb(UERROR, CONNECT_HOT_PLUG, NULL, 4000);

	if (sl != NULL)
	{
		std::cout << "STlink detected!" << std::endl;

		if (stlink_enter_swd_mode(sl) != 0 || stlink_target_connect(sl, CONNECT_HOT_PLUG) != 0)
		{
			stop();
			lastErrorMsg = "STM32 target not found!";
			return false;
		}

		lastErrorMsg = "";
		return true;
	}

	std::cout << "STLink not detected!" << std::endl;
	lastErrorMsg = "STLink not found!";
	return false;
}
bool VarReader::stop()
{
	readerState = state::STOP;
	stlink_close(sl);
	return true;
}

float VarReader::getFloat(uint32_t address, Variable::type type)
{
	volatile uint32_t value = 0;

	if (sl == nullptr)
		return 0.0f;

	uint8_t shouldShift = address % 4;

	std::lock_guard<std::mutex> lock(mtx);

	stlink_read_debug32(sl, address, (uint32_t*)&value);

	if (type == Variable::type::I8 || type == Variable::type::U8)
	{
		if (shouldShift == 0)
			value = (value & 0x000000ff);
		else if (shouldShift == 1)
			value = (value & 0x0000ff00) >> 8;
		else if (shouldShift == 2)
			value = (value & 0x00ff0000) >> 16;
		else if (shouldShift == 3)
			value = (value & 0xff000000) >> 24;
	}
	else if (type == Variable::type::I16 || type == Variable::type::U16)
	{
		if (shouldShift == 0)
			value = (value & 0x0000ffff);
		else if (shouldShift == 1)
			value = (value & 0x00ffff00) >> 8;
		else if (shouldShift == 2)
			value = (value & 0xffff0000) >> 16;
		else if (shouldShift == 3)
			value = (value & 0x000000ff) << 8 | (value & 0xff000000) >> 24;
	}
	else if (type == Variable::type::I32 || type == Variable::type::U32 || type == Variable::type::F32)
	{
		if (shouldShift == 0)
			value = value;
		else if (shouldShift == 1)
			value = (value & 0x000000ff) << 24 | (value & 0xffffff00) >> 8;
		else if (shouldShift == 2)
			value = (value & 0x0000ffff) << 16 | (value & 0xffff0000) >> 16;
		else if (shouldShift == 3)
			value = (value & 0x00ffffff) << 24 | (value & 0xff000000) >> 8;
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
	else if (type == Variable::type::UNKNOWN)
		return (float)*(uint32_t*)&value;

	return 0.0f;
}

bool VarReader::setValue(const Variable& var, float value)
{
	if (sl == nullptr)
		return false;

	uint32_t address = var.getAddress();
	int32_t retVal = 0;

	std::lock_guard<std::mutex> lock(mtx);

	switch (var.getType())
	{
		case Variable::type::U8:
			sl->q_buf[0] = static_cast<uint8_t>(value);
			retVal = stlink_write_mem8(sl, address, 1);
			break;
		case Variable::type::I8:
			sl->q_buf[0] = static_cast<int8_t>(value);
			retVal = stlink_write_mem8(sl, address, 1);
			break;
		case Variable::type::U16:
			sl->q_buf[0] = static_cast<uint16_t>(value);
			sl->q_buf[1] = static_cast<uint16_t>(value) >> 8;
			retVal = stlink_write_mem8(sl, address, 2);
			break;
		case Variable::type::I16:
			sl->q_buf[0] = static_cast<int16_t>(value);
			sl->q_buf[1] = static_cast<int16_t>(value) >> 8;
			retVal = stlink_write_mem8(sl, address, 2);
			break;
		case Variable::type::U32:
			sl->q_buf[0] = static_cast<uint32_t>(value);
			sl->q_buf[1] = static_cast<uint32_t>(value) >> 8;
			sl->q_buf[2] = static_cast<uint32_t>(value) >> 16;
			sl->q_buf[3] = static_cast<uint32_t>(value) >> 24;
			retVal = stlink_write_mem8(sl, address, 4);
			break;
		case Variable::type::I32:
			sl->q_buf[0] = static_cast<uint32_t>(value);
			sl->q_buf[1] = static_cast<uint32_t>(value) >> 8;
			sl->q_buf[2] = static_cast<uint32_t>(value) >> 16;
			sl->q_buf[3] = static_cast<uint32_t>(value) >> 24;
			retVal = stlink_write_mem8(sl, address, 4);
			break;
		case Variable::type::F32:
			sl->q_buf[0] = (*(uint32_t*)&value);
			sl->q_buf[1] = (*(uint32_t*)&value) >> 8;
			sl->q_buf[2] = (*(uint32_t*)&value) >> 16;
			sl->q_buf[3] = (*(uint32_t*)&value) >> 24;
			retVal = stlink_write_mem8(sl, address, 4);
			break;
		default:
			return false;
	}

	return retVal == 0 ? true : false;
}

std::string VarReader::getLastErrorMsg() const
{
	return lastErrorMsg;
}