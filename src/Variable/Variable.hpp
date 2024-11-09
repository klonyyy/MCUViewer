#ifndef __VARIABLE_HPP
#define __VARIABLE_HPP

#include <cstdint>
#include <functional>
#include <string>

class Variable
{
   public:
	enum class Type
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

	enum class HighLevelType
	{
		NONE = 0,
		SIGNEDFRAC = 1,
		UNSIGNEDFRAC = 2,
	};

	struct Color
	{
		float r;
		float g;
		float b;
		float a;
	};

	struct Fractional
	{
		uint32_t fractionalBits = 15;
		double base = 1.0;
	};

	explicit Variable(std::string name);
	Variable(std::string name, Type type, double value);

	void setType(Type type);
	Type getType() const;
	std::string getTypeStr() const;

	void setRawValueAndTransform(uint32_t value);
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

	bool getIsTrackedNameDifferent() const;
	void setIsTrackedNameDifferent(bool isDifferent);

	std::string getTrackedName() const;
	void setTrackedName(const std::string& trackedName);

	void setShift(uint32_t shift);
	uint32_t getShift() const;

	void setMask(uint32_t mask);
	uint32_t getMask() const;

	void setHighLevelType(HighLevelType type);
	HighLevelType getHighLevelType() const;

	void setFractional(Fractional fractional);
	Variable::Fractional getFractional() const;

   private:
	double getDoubleFromRaw();

   public:
	static const char* types[8];
	static const char* highLevelTypes[3];
	std::function<void(const std::string&, const std::string&)> renameCallback;

   private:
	std::string name = "";
	std::string trackedName = "";
	Type type = Type::UNKNOWN;
	HighLevelType highLevelType = HighLevelType::NONE;

	double value = 0.0;
	uint32_t rawValue = 0;

	uint32_t address = 0x20000000;
	Fractional fractional{};

	uint32_t shift = 0;
	uint32_t mask = 0xffffffff;

	Color color{};
	bool isFound = false;
	bool shouldUpdateFromElf = true;
	bool isTrackedNameDifferent = false;
};

#endif