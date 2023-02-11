#include "ElfReader.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

ElfReader::ElfReader(std::string& filename)
{
	elfname = filename;
}

uint32_t ElfReader::getVariableAddress(std::string& varName)
{
	std::string cmdFull(std::string("gdb -batch -ex ") + "\"file " + elfname + "\" " + "-ex " + "\"p /d &" + varName + "\" ");
	std::string out = exec(cmdFull.c_str());
	return atoi((out.substr(out.find("= ") + 2, out.length())).c_str());
}

std::vector<uint32_t> ElfReader::getVariableAddressBatch(std::vector<std::string>& varNames)
{
	std::string cmdFull(std::string("gdb -batch -ex ") + "\"file " + elfname + "\" ");

	for (auto& name : varNames)
		cmdFull += (std::string("-ex ") + "\"p /d &" + name + "\" ");

	std::string out = exec(cmdFull.c_str());
	std::string delimiter = "= ";
	int32_t pos = 0;
	int32_t pos2 = 0;

	std::vector<uint32_t> addresses;

	while (out.length() > 0 && (pos = out.find(delimiter)) != -1)
	{
		if ((pos2 = out.find('$', 1)) == -1)
			pos2 = out.length();
		addresses.push_back(atoi((out.substr(pos + delimiter.length(), pos2)).c_str()));
		out.erase(0, pos2);
	}
	return addresses;
}

std::string ElfReader::exec(const char* cmd)
{
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe)
		throw std::runtime_error("popen() failed!");
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
		result += buffer.data();
	return result;
}