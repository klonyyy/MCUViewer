#ifndef __VARIABLE_HPP
#define __VARIABLE_HPP

#include <string>

class Variable
{
   public:
	enum class type
	{
		UNKNOWN = 0,
		U8 = 1,
		I8 = 2,
		U16 = 3,
		I16 = 4,
		U32 = 5,
		I32 = 6,
		F32 = 7

	};

	struct Color
	{
		float r;
		float g;
		float b;
		float a;
	};

	Variable(std::string name);
	template <typename T>
	Variable(std::string name, type type_, T value_);
	~Variable() = default;

	void setType(type varType_);
	type getType();
	std::string getTypeStr();

	template <typename T>
	void setValue(T val);
	template <typename T>
	T getValue();

	void setAddress(uint32_t addr);
	uint32_t getAddress();
	std::string& getName();
	void setName(std::string name_);

	void setColor(float r, float g, float b, float a);
	Color& getColor();

   private:
	std::string name;
	type varType;
	uint32_t value;
	uint32_t address;
	const char* types[8] = {"UNKNOWN",
							"U8",
							"I8",
							"U16",
							"I16",
							"U32",
							"I32",
							"F32"};

	Color color;
};

#endif