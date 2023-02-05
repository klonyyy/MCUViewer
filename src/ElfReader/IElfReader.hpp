
#ifndef _IELFREADER_HPP
#define _IELFREADER_HPP

#include <cstdio>
#include <string>
#include <vector>

class IElfReader
{
   public:
	typedef struct varInfo
	{
		std::string varName;
		std::string type;
		uint32_t address;
	} varInfo;

	IElfReader() = default;
	~IElfReader() = default;

	virtual uint32_t getVariableAddress(std::string& varName) = 0;

   private:
	varInfo dupa;
};

#endif