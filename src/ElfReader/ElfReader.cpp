#include "ElfReader.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#if defined(unix) || defined(__unix__) || defined(__unix)
#define _UNIX
#endif

ElfReader::ElfReader(std::string& filename) : elfname(filename)
{
}

std::vector<uint32_t> ElfReader::getVariableAddressBatch(std::vector<std::string>& varNames)
{
	std::string cmdFull(std::string("gdb -batch -ex \"set trace-commands on\" -ex ") + "\"file " + elfname + "\" ");

	for (auto& name : varNames)
	{
		cmdFull += (std::string("-ex ") + "\"p /d &" + name + "\" ");
		cmdFull += (std::string("-ex ") + "\"ptype " + name + "\" ");
	}

	std::cout << "command :" << cmdFull.c_str() << std::endl;

	std::string out = exec(cmdFull.c_str());
	std::string delimiter = "= ";
	int32_t pos = 0;
	int32_t pos2 = 0;

	std::vector<uint32_t> addresses;

	std::cout << out << std::endl;

	while (out.length() > 0 && (pos = out.find(delimiter)) != -1)
	{
		if ((pos2 = out.find('$', 1)) == -1)
			pos2 = out.length();
		addresses.push_back(atoi((out.substr(pos + delimiter.length(), pos2)).c_str()));
		out.erase(0, pos2);
	}
	return addresses;
}

bool ElfReader::updateVariableMap(std::map<std::string, std::shared_ptr<Variable>>& vars)
{
	std::cout << elfname << std::endl;

	std::string cmdFull(std::string("gdb -batch -ex \"set trace-commands on\" -ex ") + "\"file " + elfname + "\" ");

	for (auto& [name, var] : vars)
	{
		cmdFull += (std::string("-ex ") + "\"p /d &" + name + "\" ");
		cmdFull += (std::string("-ex ") + "\"ptype " + name + "\" ");
	}

	std::string out = exec(cmdFull.c_str());
	std::string delimiter = "+p /d &";
	int32_t start = 0;
	/* get rid of file and other start commands */
	out.erase(0, out.find(delimiter));

	while (out.length() > 0 && (start = out.find(delimiter)) != -1)
	{
		std::string::size_type end = out.find(delimiter, start + 1);
		if (end == std::string::npos)
			end = out.length();

		std::string temp = out.substr(start, end - start);

		std::string::size_type addrPos = temp.find('$', 0);
		std::string::size_type typePos = temp.find("type = ", 0);

		if (addrPos < end && typePos < end && addrPos > 0 && typePos > 0)
		{
			std::string varName = temp.substr(delimiter.length(), temp.find('$', 0) - delimiter.length() - 1);
			std::shared_ptr<Variable> var = vars[varName];

			vars[varName]->setAddress(atoi(temp.substr(addrPos + 5, temp.find('\n', addrPos)).c_str()));
			std::string type = temp.substr(typePos + 7, temp.find('\n', typePos));
			vars[varName]->setType(getTypeFromString(type));
			std::cout << "NAME: " << vars[varName]->getName() << std::endl;
			std::cout << "ADDRESS: " << vars[varName]->getAddress() << std::endl;
			std::cout << "TYPE: " << unsigned((int)vars[varName]->getType()) << std::endl;
		}
		out.erase(0, temp.length());
	}
	return true;
}

Variable::type ElfReader::getTypeFromString(std::string strType)
{
	const std::vector<std::pair<std::string, Variable::type>> typeVector = {
		{"unsigned 8-bit", Variable::type::U8},
		{"unsigned char", Variable::type::U8},
		{"bool", Variable::type::U8},
		{"signed 8-bit", Variable::type::I8},
		{"signed char", Variable::type::I8},
		{"unsigned 16-bit", Variable::type::U16},
		{"unsigned short", Variable::type::U16},
		{"signed 16-bit", Variable::type::I16},
		{"signed short", Variable::type::I16},
		{"unsigned 32-bit", Variable::type::U32},
		{"unsigned int", Variable::type::U32},
		{"unsigned long", Variable::type::U32},
		{"volatile int", Variable::type::I32},
		{"signed 32-bit", Variable::type::I32},
		{"signed int", Variable::type::I32},
		{"signed long", Variable::type::I32},
		{"float", Variable::type::F32},
		{"short", Variable::type::I16},
		{"long", Variable::type::I32}};

	std::cout << "TYPE: " << strType << std::endl;

	for (auto entry : typeVector)
	{
		if (strType.find(entry.first) != std::string::npos)
		{
			return entry.second;
		}
	}
	return Variable::type::UNKNOWN;
}

std::string ElfReader::exec(const char* cmd)
{
	std::array<char, 128> buffer;
	std::string result;

#ifdef _UNIX
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
#elif _WIN32
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
#else
#error "Your system is not supported!"
#endif

	if (!pipe)
		throw std::runtime_error("popen() failed!");
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
		result += buffer.data();
	return result;
}