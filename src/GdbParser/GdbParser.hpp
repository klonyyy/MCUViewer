#ifndef _GDBPARSER_HPP
#define _GDBPARSER_HPP

#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ProcessHandler.hpp"
#include "Variable.hpp"

#ifdef _WIN32
using CurrentPlatform = WindowsProcessHandler;
#else
using CurrentPlatform = PosixProcessHandler;
#endif

class GdbParser
{
	struct VariableData
	{
		std::string name;
		uint32_t address;
		std::string filePath;
		bool isTrivial = false;
	};

   public:
	bool parse(std::string elfPath)
	{
		std::string cmd = std::string("gdb --interpreter=mi ") + elfPath;
		process.executeCmd(cmd);
		auto out = process.executeCmd("info variables\n");

		size_t start = 0;
		while (out.length() > 0)
		{
			std::string delimiter = "File";

			auto end = out.find(delimiter, start);
			if (end == std::string::npos)
				break;
			/* find tylda sign next */
			start = out.find("~", end);
			if (start == std::string::npos)
				break;
			/* account for tylda and " */
			start += 2;
			/* find the end of filepath */
			end = out.find(":", start);
			if (end == std::string::npos)
				break;

			auto filename = out.substr(start, end - start);

			auto end1 = out.find("~\"\\n", end);
			start = end;

			if (end1 != std::string::npos)
			{
				end = end1;
				auto variableChunk = out.substr(start, end - start);
				// std::cout << "File found! " << filename << std::endl;
				// std::cout << "variable chunk: " << variableChunk << std::endl;
				parseVariableChunk(variableChunk);
			}
			start = end;
		}

		for (auto& [name, address, path, trivial] : parsedData)
		{
			std::cout << name << " is trivial: " << trivial << std::endl;
		}

		return true;
	}

	void parseVariableChunk(std::string& chunk)
	{
		size_t start = 0;

		while (1)
		{
			auto semicolonPos = chunk.find(';', start);
			if (semicolonPos == std::string::npos)
				break;

			auto spacePos = chunk.rfind(' ', semicolonPos);
			if (spacePos == std::string::npos)
				break;

			std::string variableName = chunk.substr(spacePos + 1, semicolonPos - spacePos - 1);

			checkVariableType(variableName);
			start = semicolonPos + 1;
		}
	}

	void parseUntiltrivial()
	{
	}

	void checkVariableType(std::string& name)
	{
		auto out = process.executeCmd(std::string("ptype ") + name + std::string("\n"));
		auto start = out.find("=");
		auto end = out.find("\\n", start);

		auto line = out.substr(start + 2, end - start - 2);

		/* remove const and volatile */
		if (line.find("volatile ", 0) != std::string::npos)
			line.erase(0, 9);
		if (line.find("const ", 0) != std::string::npos)
			line.erase(0, 6);
		if (line.find("static const ", 0) != std::string::npos)
			line.erase(0, 13);

		bool isTrivial = checkTrivial(line);

		if (isTrivial)
			parsedData.push_back({name, 0, "", isTrivial});
		else
		{
			auto subStart = 0;

			while (1)
			{
				auto semicolonPos = out.find(';', subStart);
				if (semicolonPos == std::string::npos)
					break;

				if (out[semicolonPos - 1] == ')')
				{
					subStart = subStart = semicolonPos + 1;
					continue;
				}

				auto spacePos = out.rfind(' ', semicolonPos);

				if (spacePos == std::string::npos)
					break;

				auto varName = out.substr(spacePos + 1, semicolonPos - spacePos - 1);

				if (varName == "const")
				{
					subStart = subStart = semicolonPos + 1;
					continue;
				}

				if (varName[0] == '*')
					varName.erase(0);

				auto fullName = name + "." + varName;

				checkVariableType(fullName);
				subStart = semicolonPos + 1;
			}
		}
	}

	bool checkTrivial(std::string& line)
	{
		if (isTrivial.contains(line))
			return true;
		std::cout << line << std::endl;
		return false;
	}

	std::vector<VariableData> getParsedData()
	{
		return parsedData;
	}

   private:
	std::vector<VariableData> parsedData;
	ProcessHandler<CurrentPlatform> process;

	std::unordered_map<std::string, Variable::type> isTrivial = {
		{"bool", Variable::type::U8},
		{"unsigned char", Variable::type::U8},

		{"char", Variable::type::I8},
		{"signed char", Variable::type::I8},

		{"unsigned short", Variable::type::U16},
		{"unsigned short int", Variable::type::U16},
		{"short unsigned int", Variable::type::U16},

		{"short", Variable::type::I16},
		{"short int", Variable::type::I16},
		{"signed short", Variable::type::I16},
		{"signed short int", Variable::type::I16},
		{"short signed int", Variable::type::I16},

		{"unsigned int", Variable::type::U32},
		{"unsigned long", Variable::type::U32},
		{"unsigned long int", Variable::type::U32},
		{"long unsigned int", Variable::type::U32},

		{"int", Variable::type::I32},
		{"long", Variable::type::I32},
		{"long int", Variable::type::I32},
		{"signed long", Variable::type::I32},
		{"signed long int", Variable::type::I32},
		{"long signed int", Variable::type::I32},

		{"float", Variable::type::F32},
	};
};

#endif