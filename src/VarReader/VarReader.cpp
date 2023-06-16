#include "VarReader.hpp"

#include <map>

#include "iostream"

VarReader::VarReader(IVariableReader* variableReader, std::shared_ptr<spdlog::logger> logger) : variableReader(variableReader), logger(logger)
{
}

bool VarReader::start()
{
	return variableReader->startAcqusition();
}
bool VarReader::stop()
{
	return variableReader->stopAcqusition();
}

double VarReader::getValue(uint32_t address, Variable::type type)
{
	volatile uint32_t value = 0;
	uint8_t shouldShift = address % 4;

	std::lock_guard<std::mutex> lock(mtx);

	if (!variableReader->readMemory(address, (uint32_t*)&value))
		return 0.0;

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
		return (double)*(uint8_t*)&value;
	else if (type == Variable::type::I8)
		return (double)*(int8_t*)&value;
	else if (type == Variable::type::U16)
		return (double)*(uint16_t*)&value;
	else if (type == Variable::type::I16)
		return (double)*(int16_t*)&value;
	else if (type == Variable::type::U32)
		return (double)*(uint32_t*)&value;
	else if (type == Variable::type::I32)
		return (double)*(int32_t*)&value;
	else if (type == Variable::type::F32)
		return (double)*(float*)&value;
	else if (type == Variable::type::UNKNOWN)
		return (double)*(uint32_t*)&value;

	return 0.0;
}

bool VarReader::setValue(const Variable& var, double value)
{
	uint32_t address = var.getAddress();
	int32_t retVal = 0;
	uint8_t buf[4] = {};

	if (!variableReader->isValid())
		return false;

	std::lock_guard<std::mutex> lock(mtx);

	switch (var.getType())
	{
		case Variable::type::U8:
		{
			buf[0] = static_cast<uint8_t>(value);
			retVal = variableReader->writeMemory(address, buf, 1);
			break;
		}
		case Variable::type::I8:
		{
			buf[0] = static_cast<int8_t>(value);
			retVal = variableReader->writeMemory(address, buf, 1);
			break;
		}
		case Variable::type::U16:
		{
			uint16_t val = static_cast<uint16_t>(value);
			buf[0] = val;
			buf[1] = val >> 8;
			retVal = variableReader->writeMemory(address, buf, 2);
			break;
		}
		case Variable::type::I16:
		{
			int16_t val = static_cast<int16_t>(value);
			buf[0] = val;
			buf[1] = val >> 8;
			retVal = variableReader->writeMemory(address, buf, 2);
			break;
		}
		case Variable::type::U32:
		{
			uint32_t val = static_cast<uint32_t>(value);

			buf[0] = val;
			buf[1] = val >> 8;
			buf[2] = val >> 16;
			buf[3] = val >> 24;
			retVal = variableReader->writeMemory(address, buf, 4);
			break;
		}
		case Variable::type::I32:
		{
			int32_t val = static_cast<int32_t>(value);

			buf[0] = val;
			buf[1] = val >> 8;
			buf[2] = val >> 16;
			buf[3] = val >> 24;
			retVal = variableReader->writeMemory(address, buf, 4);
			break;
		}
		case Variable::type::F32:
		{
			float valf = static_cast<float>(value);
			uint32_t val = *(uint32_t*)&valf;

			buf[0] = val;
			buf[1] = val >> 8;
			buf[2] = val >> 16;
			buf[3] = val >> 24;
			retVal = variableReader->writeMemory(address, buf, 4);
			break;
		}
		default:
			return false;
	}

	return retVal == 0 ? true : false;
}

std::string VarReader::getLastErrorMsg() const
{
	return variableReader->getLastErrorMsg();
}