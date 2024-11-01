#ifndef __VARIABLE_HPP
#define __VARIABLE_HPP

#include <cstdint>
#include <functional>
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

	explicit Variable(std::string name);
	Variable(std::string name, type type, double value);

	void setType(type varType);
	type getType() const;
	std::string getTypeStr() const;

	void setValue(double val);
	double getValue() const;

	void setAddress(uint32_t addr);
	uint32_t getAddress() const;
	std::string& getName();
	void rename(const std::string& newName);

	void setColor(float r, float g, float b, float a);
	void setColor(uint32_t AaBbGgRr);

	Color& getColor();
	uint32_t getColorU32() const;

	bool getIsFound() const;
	void setIsFound(bool found);

	uint8_t getSize();

	bool getShouldUpdateFromElf() const;
	void setShouldUpdateFromElf(bool shouldUpdateFromElf);

	std::string getTrackedName() const;
	void setTrackedName(const std::string& trackedName);

	void setShift(uint32_t shift);
	uint32_t getShift() const;

	void setMask(uint32_t mask);
	uint32_t getMask() const;

   public:
	static const char* types[8];
	std::function<void(const std::string&, const std::string&)> renameCallback;

   private:
	std::string name;
	std::string trackedName = "";
	type varType;
	double value;
	uint32_t address;

	uint32_t shift = 0;
	uint32_t mask = 0xffffffff;

	Color color;
	bool isFound = false;
	bool shouldUpdateFromElf = true;
};

#endif