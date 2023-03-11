#ifndef _CONFIGHANDLER_HPP
#define _CONFIGHANDLER_HPP

#include <string>

#include "PlotHandler.hpp"
#include "Variable.hpp"
#include "ini.h"

class ConfigHandler
{
   public:
	ConfigHandler(std::string configFilePath, PlotHandler* plotHandler);
	~ConfigHandler() = default;

	bool changeConfigFile(std::string newConfigFilePath);
	std::string getElfFilePath();
	bool readConfigFile(std::vector<Variable>& vars, std::string& elfPath);

   private:
	std::string configFilePath;
	PlotHandler* plotHandler;

	std::unique_ptr<mINI::INIFile> file;
	std::unique_ptr<mINI::INIStructure> ini;
};

#endif