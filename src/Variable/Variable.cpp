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

Variable::Variable(std::string name, Variable::Type type, double value) : name(name), type(type), value(value)
{
	name.reserve(100);
}

void Variable::setType(Type type)
{
	this->type = type;
}

Variable::Type Variable::getType() const
{
	return type;
}

std::string Variable::getTypeStr() const
{
	return std::string(types[static_cast<uint8_t>(type)]);
}

void Variable::setRawValueAndTransform(uint32_t rawValue)
{
	/* TODO I dont like this solution */
	this->rawValue = rawValue;
	value = getDoubleFromRaw();
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

std::string Variable::getName()
{
	return name;
}

void Variable::rename(const std::string& newName)
{
	name = newName;

	if (!isTrackedNameDifferent)
		trackedName = newName;
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
	switch (type)
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

double Variable::getDoubleFromRaw()
{
	rawValue = (rawValue >> shift) & mask;

	uint32_t size = getSize();

	if (HighLevelType::SIGNEDFRAC == highLevelType)
	{
		switch (size)
		{
			case 1:
			{
				int8_t value = rawValue & 0xff;
				return (static_cast<double>(value) / (1 << (fractional.fractionalBits))) * fractional.base;
			}
			case 2:
			{
				int16_t value = rawValue & 0xffff;
				return (static_cast<double>(value) / (1 << (fractional.fractionalBits))) * fractional.base;
			}
			case 4:
			{
				int32_t value = rawValue;
				return (static_cast<double>(value) / (1 << (fractional.fractionalBits))) * fractional.base;
			}
		}
	}
	else if (HighLevelType::UNSIGNEDFRAC == highLevelType)
	{
		switch (size)
		{
			case 1:
			{
				uint8_t value = rawValue & 0xff;
				return (static_cast<double>(value) / (1 << fractional.fractionalBits)) * fractional.base;
			}
			case 2:
			{
				uint16_t value = rawValue & 0xffff;
				return (static_cast<double>(value) / (1 << fractional.fractionalBits)) * fractional.base;
			}
			case 4:
			{
				uint32_t value = rawValue;
				return (static_cast<double>(value) / (1 << fractional.fractionalBits)) * fractional.base;
			}
		}
	}

	switch (type)
	{
		case Variable::Type::U8:
			return static_cast<double>(*reinterpret_cast<uint8_t*>(&rawValue));
		case Variable::Type::I8:
			return static_cast<double>(*reinterpret_cast<int8_t*>(&rawValue));
		case Variable::Type::U16:
			return static_cast<double>(*reinterpret_cast<uint16_t*>(&rawValue));
		case Variable::Type::I16:
			return static_cast<double>(*reinterpret_cast<int16_t*>(&rawValue));
		case Variable::Type::U32:
			return static_cast<double>(*reinterpret_cast<uint32_t*>(&rawValue));
		case Variable::Type::I32:
			return static_cast<double>(*reinterpret_cast<int32_t*>(&rawValue));
		case Variable::Type::F32:
			return static_cast<double>(*reinterpret_cast<float*>(&rawValue));
		default:
			return static_cast<double>(*reinterpret_cast<uint32_t*>(&rawValue));
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

bool Variable::getIsTrackedNameDifferent() const
{
	return isTrackedNameDifferent;
}

void Variable::setIsTrackedNameDifferent(bool isDifferent)
{
	this->isTrackedNameDifferent = isDifferent;
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