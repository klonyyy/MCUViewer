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
				std::cout << "File found! " << filename << std::endl;
				std::cout << "variable chunk: " << variableChunk << std::endl;
				parseVariableChunk(variableChunk);
			}
			start = end;
		}

		checkVariableType();

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
			parsedData.push_back({variableName, 0, ""});
			start = semicolonPos + 1;
		}
	}

	void checkVariableType()
	{
		for (auto& [name, address, path, trivial] : parsedData)
		{
			auto out = process.executeCmd(std::string("ptype ") + name + std::string("\n"));
			auto start = out.find("=");
			auto end = out.find("\\n", start);

			auto line = out.substr(start + 2, end - start - 2);

			trivial = isTrivial(line);
		}
	}

	bool isTrivial(std::string& line)
	{
		std::cout << line << std::endl;
		return true;
	}

	std::vector<VariableData> getParsedData()
	{
		return parsedData;
	}

   private:
	std::vector<VariableData> parsedData;
	ProcessHandler<CurrentPlatform> process;
};

#endif