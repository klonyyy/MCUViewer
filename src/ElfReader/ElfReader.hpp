
#ifndef _ELFREADER_HPP
#define _ELFREADER_HPP

#include "IElfReader.hpp"
#include "Variable.hpp"

class ElfReader : public IElfReader
{
   public:
	ElfReader() = default;
	ElfReader(std::string& filename);
	~ElfReader() = default;

	std::vector<uint32_t> getVariableAddressBatch(std::vector<std::string>& varNames);
	std::vector<Variable> getVariableVectorBatch(std::vector<std::string>& varNames);
	Variable::type getTypeFromString(std::string strType);

   private:
	std::string& elfname;
	std::string exec(const char* cmd);
};

#endif