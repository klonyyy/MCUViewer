#include "Variable.hpp"

#include <limits>

const char* Variable::types[8] = {"unknown",
								  "uint8_t",
								  "int8_t",
								  "uint16_t",
								  "int16_t",
								  "uint32_t",
								  "int32_t",
								  "float"};

const char* Variable::highLevelTypes[3] = {"-",
										   "signed fixed point",
										   "unsigned fixed point"};

Variable::Variable(std::string name) : name(name)
{
	name.reserve(100);
}

Variable::Variable(std::string name, Variable::Type type, double value) : name(name), varType(type), value(value)
{
	name.reserve(100);
}

void Variable::setType(Type varType)
{
	this->varType = varType;
}

Variable::Type Variable::getType() const
{
	return varType;
}

std::string Variable::getTypeStr() const
{
	return std::string(types[static_cast<uint8_t>(varType)]);
}

void Variable::setValue(double val)
{
	value = val;
}

double Variable::getValue() const
{
	return value;
}

void Variable::setAddress(uint32_t addr)
{
	address = addr;
}
uint32_t Variable::getAddress() const
{
	return address;
}

std::string& Variable::getName()
{
	return name;
}

void Variable::rename(const std::string& newName)
{
	if (renameCallback)
		renameCallback(name, newName);
	name = newName;
}

void Variable::setColor(float r, float g, float b, float a)
{
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
}

void Variable::setColor(uint32_t AaBbGgRr)
{
	using u8 = std::numeric_limits<uint8_t>;
	color.r = static_cast<float>((AaBbGgRr & 0x000000ff) / static_cast<float>(u8::max()));
	color.g = static_cast<float>(((AaBbGgRr & 0x0000ff00) >> 8) / static_cast<float>(u8::max()));
	color.b = static_cast<float>(((AaBbGgRr & 0x00ff0000) >> 16) / static_cast<float>(u8::max()));
	color.a = static_cast<float>(((AaBbGgRr & 0xff000000) >> 24) / static_cast<float>(u8::max()));
}

Variable::Color& Variable::getColor()
{
	return color;
}

uint32_t Variable::getColorU32() const
{
	using u8 = std::numeric_limits<uint8_t>;
	uint32_t a = u8::max() * color.a;
	uint32_t r = u8::max() * color.r;
	uint32_t g = u8::max() * color.g;
	uint32_t b = u8::max() * color.b;

	return static_cast<uint32_t>((a << 24) | (b << 16) | (g << 8) | r);
}

bool Variable::getIsFound() const
{
	if (shouldUpdateFromElf)
		return isFound;
	return true;
}

void Variable::setIsFound(bool found)
{
	isFound = found;
}

uint8_t Variable::getSize()
{
	switch (varType)
	{
		case Type::U8:
		case Type::I8:
			return 1;
		case Type::U16:
		case Type::I16:
			return 2;
		case Type::U32:
		case Type::I32:
		case Type::F32:
			return 4;
		default:
			return 1;
	}
}

bool Variable::getShouldUpdateFromElf() const
{
	return shouldUpdateFromElf;
}

void Variable::setShouldUpdateFromElf(bool shouldUpdateFromElf)
{
	this->shouldUpdateFromElf = shouldUpdateFromElf;
}

std::string Variable::getTrackedName() const
{
	return trackedName;
}

void Variable::setTrackedName(const std::string& trackedName)
{
	this->trackedName = trackedName;
}

void Variable::setShift(uint32_t shift)
{
	this->shift = shift;
}

uint32_t Variable::getShift() const
{
	return shift;
}

void Variable::setMask(uint32_t mask)
{
	this->mask = mask;
}

uint32_t Variable::getMask() const
{
	return mask;
}

void Variable::setHighLevelType(HighLevelType varType)
{
	highLevelType = varType;
}

Variable::HighLevelType Variable::getHighLevelType() const
{
	return highLevelType;
}

void Variable::setFractional(Fractional fractional)
{
	this->fractional = fractional;
}

Variable::Fractional Variable::getFractional() const
{
	return fractional;
}