#ifndef __VARIABLE_HPP
#define __VARIABLE_HPP

#include <string>

class Variable
{
   public:
	enum class type
	{
		U8 = 0,
		I8 = 1,
		U16 = 2,
		I16 = 3,
		U32 = 4,
		I32 = 5,
		F32 = 6
	};

	Variable() = default;
	template <typename T>
	Variable(type type_, T value_) : varType(type_)
	{
		setValue<T>(value_);
	}
	~Variable() = default;

	void setType(type varType_)
	{
		varType = varType;
	}

	type getType()
	{
		return varType;
	}

	template <typename T>
	void setValue(T val)
	{
		value = *(uint32_t*)&val;
	}

	template <typename T>
	T getValue()
	{
		return *(T*)&value;
	}

   private:
	type varType;
	uint32_t value;
};

#endif