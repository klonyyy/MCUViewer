
#ifndef _ELFREADER_HPP
#define _ELFREADER_HPP

#include <map>
#include <memory>
#include <vector>

#include "Variable.hpp"

class ElfReader
{
   public:
	ElfReader(std::string& filename);

	bool updateVariableMap(std::map<std::string, std::shared_ptr<Variable>>& vars);
	Variable::type getTypeFromString(const std::string& strType);

   private:
	std::string& elfname;
	std::string executeCommand(const char* cmd);
};

#endif