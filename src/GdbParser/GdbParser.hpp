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
			std::string::size_type end = out.find(delimiter, start + 1);
			if (end == std::string::npos)
				break;

			std::string temp = out.substr(start, end - start);

			std::cout << "File found!" << std::endl;

			out.erase(0, temp.length());
		}

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