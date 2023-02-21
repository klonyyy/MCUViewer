#include "Variable.hpp"

Variable::Variable(std::string name) : name(name)
{
}
template <typename T>
Variable::Variable(std::string name, Variable::type type_, T value_) : name(name), varType(type_)
{
	setValue<T>(value_);
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