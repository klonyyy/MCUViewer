#ifndef _GDBPARSER_HPP
#define _GDBPARSER_HPP

#include <algorithm>
#include <array>
#include <atomic>
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
#include "VariableHandler.hpp"
#include "spdlog/spdlog.h"

class GdbParser
{
   public:
	struct VariableData
	{
		uint32_t address;
		bool isTrivial = false;
	};

	GdbParser(VariableHandler* variableHandler, spdlog::logger* logger);

	bool validateGDB();
	bool updateVariableMap(const std::string& elfPath);
	bool parse(const std::string& elfPath, std::atomic<bool>& shouldStopParsing);
	std::map<std::string, VariableData> getParsedData();

	void changeCurrentGDBCommand(const std::string& command);

   private:
	void parseVariableChunk(const std::string& chunk);
	void checkVariableType(std::string& name);
	Variable::Type checkType(const std::string& name, std::string* output);
	std::optional<uint32_t> checkAddress(const std::string& name);

   private:
	const char* defaultGDBCommand = "gdb";
	std::string currentGDBCommand = std::string(defaultGDBCommand);
	VariableHandler* variableHandler;
	spdlog::logger* logger;
	std::mutex mtx;
	std::map<std::string, VariableData> parsedData;
	ProcessHandler process;

	std::unordered_map<std::string, Variable::Type> isTrivial = {
		{"_Bool", Variable::Type::U8},
		{"bool", Variable::Type::U8},
		{"unsigned char", Variable::Type::U8},
		{"unsigned 8-bit", Variable::Type::U8},

		{"char", Variable::Type::I8},
		{"signed char", Variable::Type::I8},
		{"signed 8-bit", Variable::Type::I8},

		{"unsigned short", Variable::Type::U16},
		{"unsigned 16-bit", Variable::Type::U16},
		{"unsigned short int", Variable::Type::U16},
		{"short unsigned int", Variable::Type::U16},

		{"short", Variable::Type::I16},
		{"short int", Variable::Type::I16},
		{"signed short", Variable::Type::I16},
		{"signed 16-bit", Variable::Type::I16},
		{"signed short int", Variable::Type::I16},
		{"short signed int", Variable::Type::I16},

		{"unsigned int", Variable::Type::U32},
		{"unsigned long", Variable::Type::U32},
		{"unsigned 32-bit", Variable::Type::U32},
		{"unsigned long int", Variable::Type::U32},
		{"long unsigned int", Variable::Type::U32},

		{"int", Variable::Type::I32},
		{"long", Variable::Type::I32},
		{"long int", Variable::Type::I32},
		{"signed int", Variable::Type::I32},
		{"signed long", Variable::Type::I32},
		{"signed 32-bit", Variable::Type::I32},
		{"signed long int", Variable::Type::I32},
		{"long signed int", Variable::Type::I32},

		{"float", Variable::Type::F32},
	};
};
#endif