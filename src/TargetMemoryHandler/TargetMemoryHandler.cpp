#include "TargetMemoryHandler.hpp"

#include <map>

#include "iostream"

TargetMemoryHandler::TargetMemoryHandler(ITargetMemoryHandler* memoryHandler, std::shared_ptr<spdlog::logger> logger) : memoryHandler(memoryHandler), logger(logger)
{
}

bool TargetMemoryHandler::start()
{
	return memoryHandler->startAcqusition();
}
bool TargetMemoryHandler::stop()
{
	return memoryHandler->stopAcqusition();
}

double TargetMemoryHandler::getValue(uint32_t address, Variable::type type)
{
	volatile uint32_t value = 0;
	uint8_t shouldShift = address % 4;

	std::lock_guard<std::mutex> lock(mtx);

	if (!memoryHandler->readMemory(address, (uint32_t*)&value))
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

bool TargetMemoryHandler::setValue(const Variable& var, double value)
{
	uint32_t address = var.getAddress();
	uint8_t buf[4] = {};

	if (!memoryHandler->isValid())
		return false;

	auto prepareBufferAndWrite = [&](auto var, uint8_t* buf) -> int
	{
		for (size_t i = 0; i < sizeof(var); i++)
			buf[i] = var >> 8 * i;
		return memoryHandler->writeMemory(address, buf, sizeof(var));
	};

	std::lock_guard<std::mutex> lock(mtx);

	switch (var.getType())
	{
		case Variable::type::U8:
			return prepareBufferAndWrite(static_cast<uint8_t>(value), buf);
		case Variable::type::I8:
			return prepareBufferAndWrite(static_cast<int8_t>(value), buf);
		case Variable::type::U16:
			return prepareBufferAndWrite(static_cast<uint16_t>(value), buf);
		case Variable::type::I16:
			return prepareBufferAndWrite(static_cast<int16_t>(value), buf);
		case Variable::type::U32:
			return prepareBufferAndWrite(static_cast<uint32_t>(value), buf);
		case Variable::type::I32:
			return prepareBufferAndWrite(static_cast<int32_t>(value), buf);
		case Variable::type::F32:
		{
			float valf = static_cast<float>(value);
			uint32_t val = *(uint32_t*)&valf;
			return prepareBufferAndWrite(val, buf);
		}
		default:
			return false;
	}
}

std::string TargetMemoryHandler::getLastErrorMsg() const
{
	return memoryHandler->getLastErrorMsg();
}