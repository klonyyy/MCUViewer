#ifndef _CONFIGHANDLER_HPP
#define _CONFIGHANDLER_HPP

#include <string>
#include <map>

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
	bool readConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, std::string& elfPath);
	bool saveConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, std::string& elfPath, std::string newPath);

   private:
	std::string configFilePath;
	PlotHandler* plotHandler;

	std::unique_ptr<mINI::INIFile> file;
	std::unique_ptr<mINI::INIStructure> ini;
};

#endif