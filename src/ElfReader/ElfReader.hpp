
#ifndef _ELFREADER_HPP
#define _ELFREADER_HPP

#include <memory>
#include <map>

#include "IElfReader.hpp"
#include "Variable.hpp"

class ElfReader : public IElfReader
{
   public:
	ElfReader() = default;
	ElfReader(std::string& filename);
	~ElfReader() = default;

	std::vector<uint32_t> getVariableAddressBatch(std::vector<std::string>& varNames);
	bool updateVariableMap(std::map<std::string, std::shared_ptr<Variable>>& vars);
	Variable::type getTypeFromString(std::string strType);

   private:
	std::string& elfname;
	std::string exec(const char* cmd);
};

#endif