#ifndef __VARIABLE_HPP
#define __VARIABLE_HPP

#include <string>
#include <cstdint>
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
	Variable(std::string name, type type, double value);

	void setType(type varType);
	type getType() const;
	std::string getTypeStr() const;

	void setValue(double val);
	double getValue() const;

	void setAddress(uint32_t addr);
	uint32_t getAddress() const;
	std::string& getName();
	void setName(const std::string& name_);

	void setColor(float r, float g, float b, float a);
	void setColor(uint32_t AaBbGgRr);

	Color& getColor();
	uint32_t getColorU32() const;

	bool getIsFound() const;
	void setIsFound(bool found);

   private:
	std::string name;
	type varType;
	double value;
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
	bool isFound = false;
};

#endif