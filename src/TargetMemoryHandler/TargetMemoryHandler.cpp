#include "TargetMemoryHandler.hpp"

#include <map>
#include <memory>

#include "iostream"

bool TargetMemoryHandler::start(const std::string& serialNumber, const std::string& device) const
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->startAcqusition(serialNumber, device);
}
bool TargetMemoryHandler::stop() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->stopAcqusition();
}

double TargetMemoryHandler::getValue(uint32_t address, Variable::type type)
{
	uint32_t value = 0;
	uint8_t shouldShift = address % 4;

	std::lock_guard<std::mutex> lock(mtx);

	if (!probe->readMemory(address, reinterpret_cast<uint32_t*>(&value)))
		return 0.0;

	if (probe->requiresAlignedAccessOnRead())
	{
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
			if (shouldShift == 1)
				value = (value & 0x000000ff) << 24 | (value & 0xffffff00) >> 8;
			else if (shouldShift == 2)
				value = (value & 0x0000ffff) << 16 | (value & 0xffff0000) >> 16;
			else if (shouldShift == 3)
				value = (value & 0x00ffffff) << 24 | (value & 0xff000000) >> 8;
		}
	}

	switch (type)
	{
		case Variable::type::U8:
			return static_cast<double>(*reinterpret_cast<uint8_t*>(&value));
		case Variable::type::I8:
			return static_cast<double>(*reinterpret_cast<int8_t*>(&value));
		case Variable::type::U16:
			return static_cast<double>(*reinterpret_cast<uint16_t*>(&value));
		case Variable::type::I16:
			return static_cast<double>(*reinterpret_cast<int16_t*>(&value));
		case Variable::type::U32:
			return static_cast<double>(*reinterpret_cast<uint32_t*>(&value));
		case Variable::type::I32:
			return static_cast<double>(*reinterpret_cast<int32_t*>(&value));
		case Variable::type::F32:
			return static_cast<double>(*reinterpret_cast<float*>(&value));
		default:
			return static_cast<double>(*reinterpret_cast<uint32_t*>(&value));
	}
}

bool TargetMemoryHandler::setValue(const Variable& var, double value)
{
	uint32_t address = var.getAddress();
	uint8_t buf[4] = {};

	if (!probe->isValid())
		return false;

	auto prepareBufferAndWrite = [&](auto var, uint8_t* buf) -> int
	{
		for (size_t i = 0; i < sizeof(var); i++)
			buf[i] = var >> 8 * i;
		std::lock_guard<std::mutex> lock(mtx);
		return probe->writeMemory(address, buf, sizeof(var));
	};

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
			uint32_t val = *reinterpret_cast<uint32_t*>(&valf);
			return prepareBufferAndWrite(val, buf);
		}
		default:
			return false;
	}
}

std::string TargetMemoryHandler::getLastErrorMsg() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->getLastErrorMsg();
}

std::vector<std::string> TargetMemoryHandler::getConnectedDevices() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return probe->getConnectedDevices();
}

void TargetMemoryHandler::changeDevice(std::shared_ptr<IDebugProbe> newProbe)
{
	std::lock_guard<std::mutex> lock(mtx);
	probe = newProbe;
}