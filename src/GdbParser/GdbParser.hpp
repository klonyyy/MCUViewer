#ifndef _GDBPARSER_HPP
#define _GDBPARSER_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ProcessHandler.hpp"
#include "Variable.hpp"
#include "spdlog/spdlog.h"

class GdbParser
{
   public:
	struct VariableData
	{
		uint32_t address;
		bool isTrivial = false;
	};

	GdbParser(spdlog::logger* logger);

	bool validateGDB();
	bool updateVariableMap(const std::string& elfPath, std::map<std::string, std::shared_ptr<Variable>>& vars);
	bool parse(const std::string& elfPath);
	std::map<std::string, VariableData> getParsedData();

	void changeCurrentGDBCommand(const std::string& command);

   private:
	void parseVariableChunk(const std::string& chunk);
	void checkVariableType(std::string& name);
	Variable::type checkType(const std::string& name, std::string* output);
	std::optional<uint32_t> checkAddress(const std::string& name);

   private:
	const char* defaultGDBCommand = "gdb";
	std::string currentGDBCommand = std::string(defaultGDBCommand);
	spdlog::logger* logger;
	std::mutex mtx;
	std::map<std::string, VariableData> parsedData;
	ProcessHandler process;

	std::unordered_map<std::string, Variable::type> isTrivial = {
		{"_Bool", Variable::type::U8},
		{"bool", Variable::type::U8},
		{"unsigned char", Variable::type::U8},
		{"unsigned 8-bit", Variable::type::U8},

		{"char", Variable::type::I8},
		{"signed char", Variable::type::I8},
		{"signed 8-bit", Variable::type::I8},

		{"unsigned short", Variable::type::U16},
		{"unsigned 16-bit", Variable::type::U16},
		{"unsigned short int", Variable::type::U16},
		{"short unsigned int", Variable::type::U16},

		{"short", Variable::type::I16},
		{"short int", Variable::type::I16},
		{"signed short", Variable::type::I16},
		{"signed 16-bit", Variable::type::I16},
		{"signed short int", Variable::type::I16},
		{"short signed int", Variable::type::I16},

		{"unsigned int", Variable::type::U32},
		{"unsigned long", Variable::type::U32},
		{"unsigned 32-bit", Variable::type::U32},
		{"unsigned long int", Variable::type::U32},
		{"long unsigned int", Variable::type::U32},

		{"int", Variable::type::I32},
		{"long", Variable::type::I32},
		{"long int", Variable::type::I32},
		{"signed int", Variable::type::I32},
		{"signed long", Variable::type::I32},
		{"signed 32-bit", Variable::type::I32},
		{"signed long int", Variable::type::I32},
		{"long signed int", Variable::type::I32},

		{"float", Variable::type::F32},
	};
};
#endif