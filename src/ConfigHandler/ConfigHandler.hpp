#ifndef _CONFIGHANDLER_HPP
#define _CONFIGHANDLER_HPP

#include <map>
#include <string>

#include "PlotHandler.hpp"
#include "Variable.hpp"
#include "ini.h"

class ConfigHandler
{
   public:
	typedef struct Settings
	{
		uint32_t samplePeriod;
	}Settings;

	ConfigHandler(const std::string& configFilePath, PlotHandler* plotHandler);
	~ConfigHandler() = default;

	bool changeConfigFile(const std::string& newConfigFilePath);
	std::string getElfFilePath() const;
	bool readConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, std::string& elfPath, Settings& settings) const;
	bool saveConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, const std::string& elfPath, const Settings& settings, const std::string newPath);

   private:
	std::string configFilePath;
	PlotHandler* plotHandler;

	std::unique_ptr<mINI::INIFile> file;
	std::unique_ptr<mINI::INIStructure> ini;
};

#endif