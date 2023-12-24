
#ifndef _ELFREADER_HPP
#define _ELFREADER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Variable.hpp"
#include "spdlog/spdlog.h"

class ElfReader
{
   public:
	ElfReader(std::string& filename, spdlog::logger* logger);

	bool updateVariableMap(std::map<std::string, std::shared_ptr<Variable>>& vars);
	Variable::type getTypeFromString(const std::string& strType);

	int32_t extractGDBVersionNumber(const std::string&& versionString);

   private:
	static constexpr int32_t gdbMinimumVersion = 120;
	static constexpr uint8_t maxNameLength = 100;
	static constexpr uint16_t maxGdbCmdLendth = 8160;
	std::string& elfname;
	std::string executeCommand(const char* cmd);
	spdlog::logger* logger;
};

#endif