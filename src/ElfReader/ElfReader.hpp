
#ifndef _ELFREADER_HPP
#define _ELFREADER_HPP

#include <map>
#include <memory>
#include <vector>

#include "Variable.hpp"

class ElfReader
{
   public:
	ElfReader() = default;
	ElfReader(std::string& filename);
	~ElfReader() = default;

	std::vector<uint32_t> getVariableAddressBatch(const std::vector<std::string>& varNames);
	bool updateVariableMap(std::map<std::string, std::shared_ptr<Variable>>& vars);
	Variable::type getTypeFromString(const std::string& strType);

   private:
	std::string& elfname;
	std::string exec(const char* cmd);
};

#endif