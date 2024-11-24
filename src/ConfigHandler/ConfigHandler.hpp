#pragma once

#include <map>
#include <memory>
#include <string>

#include "PlotGroupHandler.hpp"
#include "TraceDataHandler.hpp"
#include "Variable.hpp"
#include "VariableHandler.hpp"
#include "ViewerDataHandler.hpp"
#include "ini.h"
#include "spdlog/spdlog.h"

class ConfigHandler
{
   public:
	typedef struct
	{
		uint32_t version = 1;
	} GlobalSettings;

	ConfigHandler(const std::string& configFilePath, PlotHandler* plotHandler, PlotHandler* tracePlotHandler, PlotGroupHandler* plotGroupHandler, VariableHandler* variableHandler, ViewerDataHandler* viewerDataHandler, TraceDataHandler* traceDataHandler, spdlog::logger* logger);
	~ConfigHandler() = default;

	bool changeConfigFile(const std::string& newConfigFilePath);
	bool readConfigFile(std::string& elfPath);
	bool saveConfigFile(const std::string& elfPath, const std::string& newSavePath);
	bool isSavingRequired(const std::string& elfPath);

	template <typename T>
	void parseValue(const std::string& value, T& result)
	{
		try
		{
			if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
				result = std::stoi(value);
			else if constexpr (std::is_enum_v<T>)
				result = static_cast<T>(std::stoi(value));
			else if constexpr (std::is_same_v<T, float>)
				result = std::stof(value);
			else if constexpr (std::is_same_v<T, double>)
				result = std::stod(value);
			else if constexpr (std::is_same_v<T, bool>)
				result = value == "true";
			else
				throw std::invalid_argument("Unsupported type");
		}
		catch (...)
		{
			logger->warn("stoi incorect argument: {}", value);
		}
	}

   private:
	void loadVariables();
	void loadPlots();
	void loadTracePlots();
	void loadPlotGroups();

	mINI::INIStructure prepareSaveConfigFile(const std::string& elfPath);

   private:
	GlobalSettings globalSettings;
	std::string configFilePath;
	PlotHandler* plotHandler;
	PlotHandler* tracePlotHandler;
	PlotGroupHandler* plotGroupHandler;
	VariableHandler* variableHandler;
	ViewerDataHandler* viewerDataHandler;
	TraceDataHandler* traceDataHandler;
	spdlog::logger* logger;

	std::map<std::string, Plot::displayFormat> displayFormatMap{{"DEC", Plot::displayFormat::DEC}, {"HEX", Plot::displayFormat::HEX}, {"BIN", Plot::displayFormat::BIN}};

	std::unique_ptr<mINI::INIFile> file;
	std::unique_ptr<mINI::INIStructure> ini;
};
