#include "Variable.hpp"

#include <limits>

Variable::Variable(std::string name) : name(name)
{
}
template <typename T>
Variable::Variable(std::string name, Variable::type type_, T value_) : name(name), varType(type_)
{
	setValue<T>(value_);
	name.reserve(100);
}

void Variable::setType(const type varType_)
{
	varType = varType_;
}

Variable::type Variable::getType() const
{
	return varType;
}

std::string Variable::getTypeStr() const
{
	return std::string(types[static_cast<uint8_t>(varType)]);
}

template <typename T>
void Variable::setValue(const T val)
{
	value = *(uint32_t*)&val;
}

template <typename T>
T Variable::getValue() const
{
	return *(T*)&value;
}

void Variable::setAddress(const uint32_t addr)
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

void Variable::setName(const std::string& name_)
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

void Variable::setColor(const uint32_t AaBbGgRr)
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