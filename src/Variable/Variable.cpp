#include "Variable.hpp"

#include <limits>

const char* Variable::types[8] = {"UNKNOWN",
								  "U8",
								  "I8",
								  "U16",
								  "I16",
								  "U32",
								  "I32",
								  "F32"};

Variable::Variable(std::string name) : name(name)
{
	name.reserve(100);
	value = 0.0;
	address = 0x20000000;
	varType = type::UNKNOWN;
	color = {0.0, 0.0, 0.0, 0.0};
}

Variable::Variable(std::string name, Variable::type type, double value) : name(name), varType(type), value(value)
{
	name.reserve(100);
	address = 0x20000000;
	color = {0.0, 0.0, 0.0, 0.0};
}

void Variable::setType(type varType)
{
	this->varType = varType;
}

Variable::type Variable::getType() const
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
		case type::U8:
		case type::I8:
			return 1;
		case type::U16:
		case type::I16:
			return 2;
		case type::U32:
		case type::I32:
		case type::F32:
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
