
#ifndef _ELFREADER_HPP
#define _ELFREADER_HPP

#include "IElfReader.hpp"

class ElfReader : public IElfReader
{
   public:
	ElfReader() = default;
	ElfReader(std::string& filename);
	~ElfReader() = default;

	uint32_t getVariableAddress(std::string& varName) override;
	std::vector<uint32_t> getVariableAddressBatch(std::vector<std::string>& varNames);

   private:
	std::string elfname;
	std::string exec(const char* cmd);
};

#endif