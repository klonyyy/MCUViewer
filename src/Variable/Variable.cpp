#include "Variable.hpp"

Variable::Variable(std::string name) : name(name)
{
}
template <typename T>
Variable::Variable(std::string name, Variable::type type_, T value_) : name(name), varType(type_)
{
	setValue<T>(value_);
	name.reserve(50);
}

void Variable::setType(type varType_)
{
	varType = varType_;
}

Variable::type Variable::getType()
{
	return varType;
}

std::string Variable::getTypeStr()
{
	return std::string(types[static_cast<uint8_t>(varType)]);
}

template <typename T>
void Variable::setValue(T val)
{
	value = *(uint32_t*)&val;
}

template <typename T>
T Variable::getValue()
{
	return *(T*)&value;
}

void Variable::setAddress(uint32_t addr)
{
	address = addr;
}
uint32_t Variable::getAddress()
{
	return address;
}

std::string& Variable::getName()
{
	return name;
}

void Variable::setName(std::string name_)
{
	name = name_;
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
	color.r = static_cast<float>((AaBbGgRr & 0x000000ff) / 255.0f);
	color.g = static_cast<float>(((AaBbGgRr & 0x0000ff00) >> 8) / 255.0f);
	color.b = static_cast<float>(((AaBbGgRr & 0x00ff0000) >> 16) / 255.0f);
	color.a = static_cast<float>(((AaBbGgRr & 0xff000000) >> 24) / 255.0f);
}

Variable::Color& Variable::getColor()
{
	return color;
}

uint32_t Variable::getColorU32()
{
	uint32_t a = UINT8_MAX * color.a;
	uint32_t r = UINT8_MAX * color.r;
	uint32_t g = UINT8_MAX * color.g;
	uint32_t b = UINT8_MAX * color.b;

	return static_cast<uint32_t>((a << 24) | (b << 16) | (g << 8) | r);
}