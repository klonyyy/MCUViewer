#ifndef _CONFIGHANDLER_HPP
#define _CONFIGHANDLER_HPP

#include <map>
#include <memory>
#include <string>

#include "PlotHandler.hpp"
#include "TracePlotHandler.hpp"
#include "Variable.hpp"
#include "ini.h"
#include "spdlog/spdlog.h"

class ConfigHandler
{
   public:
	typedef struct
	{
		uint32_t version = 1;
	} GlobalSettings;

	ConfigHandler(const std::string& configFilePath, PlotHandler* plotHandler, TracePlotHandler* tracePlotHandler, std::shared_ptr<spdlog::logger> logger);
	~ConfigHandler() = default;

	bool changeConfigFile(const std::string& newConfigFilePath);
	bool readConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, std::string& elfPath);
	bool saveConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, const std::string& elfPath, const std::string& newSavePath);

	template <typename T>
	void parseValue(const std::string& value, T& result)
	{
		if constexpr (std::is_integral_v<T>)
			result = std::stoi(value);
		else if constexpr (std::is_same_v<T, float>)
			result = std::stof(value);
		else
			throw std::invalid_argument("Unsupported type");
	}

   private:
	GlobalSettings globalSettings;
	std::string configFilePath;
	PlotHandler* plotHandler;
	TracePlotHandler* tracePlotHandler;

	std::map<std::string, Plot::displayFormat> displayFormatMap{{"DEC", Plot::displayFormat::DEC}, {"HEX", Plot::displayFormat::HEX}, {"BIN", Plot::displayFormat::BIN}};

	std::unique_ptr<mINI::INIFile> file;
	std::unique_ptr<mINI::INIStructure> ini;
	std::shared_ptr<spdlog::logger> logger;
};

#endif