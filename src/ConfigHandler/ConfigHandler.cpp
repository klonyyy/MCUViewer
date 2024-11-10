#include "ConfigHandler.hpp"

#include <memory>
#include <random>
#include <variant>

/* TODO refactor whole config and persistent storage handling */
ConfigHandler::ConfigHandler(const std::string& configFilePath, PlotHandler* plotHandler, TracePlotHandler* tracePlotHandler, PlotGroupHandler* plotGroupHandler, VariableHandler* variableHandler, spdlog::logger* logger)
	: configFilePath(configFilePath),
	  plotHandler(plotHandler),
	  tracePlotHandler(tracePlotHandler),
	  plotGroupHandler(plotGroupHandler),
	  variableHandler(variableHandler),
	  logger(logger)
{
	ini = std::make_unique<mINI::INIStructure>();
	file = std::make_unique<mINI::INIFile>(configFilePath);
}

bool ConfigHandler::changeConfigFile(const std::string& newConfigFilePath)
{
	configFilePath = newConfigFilePath;
	file.reset();
	file = std::make_unique<mINI::INIFile>(configFilePath);
	return true;
}

void ConfigHandler::loadVariables()
{
	uint32_t varId = 0;
	std::string name = "xxx";

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	while (!name.empty())
	{
		name = ini->get(varFieldFromID(varId)).get("name");

		std::shared_ptr<Variable> newVar = std::make_shared<Variable>(name);
		std::string trackedName = ini->get(varFieldFromID(varId)).get("tracked_name");
		if (trackedName.empty())
			trackedName = name;
		newVar->setTrackedName(trackedName);

		if (trackedName != name)
			newVar->setIsTrackedNameDifferent(true);

		newVar->setAddress(atoi(ini->get(varFieldFromID(varId)).get("address").c_str()));
		newVar->setType(static_cast<Variable::Type>(atoi(ini->get(varFieldFromID(varId)).get("type").c_str())));
		newVar->setColor(static_cast<uint32_t>(atol(ini->get(varFieldFromID(varId)).get("color").c_str())));
		newVar->setShift(atoi(ini->get(varFieldFromID(varId)).get("shift").c_str()));

		Variable::HighLevelType highLevelType = static_cast<Variable::HighLevelType>(atoi(ini->get(varFieldFromID(varId)).get("high_level_type").c_str()));
		newVar->setHighLevelType(highLevelType);

		if (highLevelType != Variable::HighLevelType::NONE)
		{
			Variable::Fractional frac = {.fractionalBits = atoi(ini->get(varFieldFromID(varId)).get("frac").c_str()),
										 .base = atof(ini->get(varFieldFromID(varId)).get("base").c_str())};
			newVar->setFractional(frac);
		}

		uint32_t mask = atoi(ini->get(varFieldFromID(varId)).get("mask").c_str());
		if (mask == 0)
			mask = 0xFFFFFFFF;
		newVar->setMask(mask);

		std::string shouldUpdateFromElf = ini->get(varFieldFromID(varId)).get("should_update_from_elf");
		if (shouldUpdateFromElf.empty())
			shouldUpdateFromElf = "true";
		newVar->setShouldUpdateFromElf(shouldUpdateFromElf == "true" ? true : false);
		varId++;

		if (newVar->getAddress() % 4 != 0)
			logger->warn("--------- Unaligned variable address! ----------");

		if (!newVar->getName().empty())
		{
			variableHandler->addVariable(newVar);
			newVar->setIsFound(true);
			logger->info("Adding variable: {}", newVar->getName());
		}
	}
}

void ConfigHandler::loadPlots()
{
	uint32_t plotNumber = 0;
	std::string plotName("xxx");

	auto plotSeriesFieldFromID = [](uint32_t plotId, uint32_t seriesId)
	{ return std::string("plot" + std::to_string(plotId) + "-" + "series" + std::to_string(seriesId)); };

	while (!plotName.empty())
	{
		std::string sectionName("plot" + std::to_string(plotNumber));
		plotName = ini->get(sectionName).get("name");
		bool visibility = ini->get(sectionName).get("visibility") == "true" ? true : false;
		Plot::Type type = static_cast<Plot::Type>(atoi(ini->get(sectionName).get("type").c_str()));

		if (!plotName.empty())
		{
			plotHandler->addPlot(plotName);
			auto plot = plotHandler->getPlot(plotName);
			plot->setVisibility(visibility);
			plot->setType(type);
			logger->info("Adding plot: {}", plotName);
			uint32_t seriesNumber = 0;
			std::string varName = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("name");

			while (varName != "")
			{
				plot->addSeries(variableHandler->getVariable(varName).get());
				bool visible = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("visibility") == "true" ? true : false;
				plot->getSeries(varName)->visible = visible;
				std::string displayFormat = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("format");
				if (displayFormat == "")
					displayFormat = "DEC";
				plot->getSeries(varName)->format = displayFormatMap.at(displayFormat);
				logger->info("Adding series: {}", varName);
				seriesNumber++;
				varName = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("name");
			}
		}
		plotNumber++;
	}
}
void ConfigHandler::loadTracePlots()
{
	std::string plotName = "xxx";
	uint32_t plotNumber = 0;
	const uint32_t colors[] = {4294967040, 4294960666, 4294954035, 4294947661, 4294941030, 4294934656, 4294928025, 4294921651, 4294915020, 4294908646, 4294902015};
	const uint32_t colormapSize = sizeof(colors) / sizeof(colors[0]);

	while (!plotName.empty())
	{
		std::string sectionName("trace_plot" + std::to_string(plotNumber));
		plotName = ini->get(sectionName).get("name");
		bool visibility = ini->get(sectionName).get("visibility") == "true" ? true : false;
		Plot::Domain domain = static_cast<Plot::Domain>(atoi(ini->get(sectionName).get("domain").c_str()));
		Plot::TraceVarType traceVarType = static_cast<Plot::TraceVarType>(atoi(ini->get(sectionName).get("type").c_str()));
		std::string alias = ini->get(sectionName).get("alias");

		if (!plotName.empty())
		{
			tracePlotHandler->addPlot(plotName);
			auto plot = tracePlotHandler->getPlot(plotName);
			plot->setVisibility(visibility);
			plot->setDomain(domain);
			if (domain == Plot::Domain::ANALOG)
				plot->setTraceVarType(traceVarType);
			plot->setAlias(alias);
			logger->info("Adding trace plot: {}", plotName);

			auto newVar = std::make_shared<Variable>(plotName);
			newVar->setColor(colors[(colormapSize - 1) - (plotNumber % colormapSize)]);
			tracePlotHandler->traceVars[plotName] = newVar;
			plot->addSeries(newVar.get());
			plot->getSeries(plotName)->visible = true;
		}
		plotNumber++;
	}
}

void ConfigHandler::loadPlotGroups()
{
	uint32_t groupNumber = 0;
	std::string groupName("xxx");

	auto plotGroupFieldFromID = [](uint32_t groupId, uint32_t plotId, const std::string& prefix = "")
	{ return std::string(prefix + "group" + std::to_string(groupId) + "-" + "plot" + std::to_string(plotId)); };

	plotGroupHandler->removeAllGroups();

	while (!groupName.empty())
	{
		std::string sectionName("group" + std::to_string(groupNumber));
		groupName = ini->get(sectionName).get("name");

		if (!groupName.empty())
		{
			auto group = plotGroupHandler->addGroup(groupName);
			logger->info("Adding group: {}", groupName);
			uint32_t plotNumber = 0;
			std::string plotName = ini->get(plotGroupFieldFromID(groupNumber, plotNumber)).get("name");

			while (plotName != "")
			{
				group->addPlot(plotHandler->getPlot(plotName));
				logger->info("Adding plot {} to group {}", plotName, groupName);

				plotNumber++;
				plotName = ini->get(plotGroupFieldFromID(groupNumber, plotNumber)).get("name");
			}
		}
		groupNumber++;
	}
	/* Add all plots to the first group if there are no groups */
	if (plotGroupHandler->getGroupCount() == 0)
	{
		std::string groupName = "default group";
		auto group = plotGroupHandler->addGroup(groupName);
		plotGroupHandler->setActiveGroup(groupName);
		logger->info("Adding group: {}", groupName);

		for (std::shared_ptr<Plot> plot : *plotHandler)
			group->addPlot(plot);
	}
}

bool ConfigHandler::readConfigFile(std::string& elfPath)
{
	PlotHandler::Settings viewerSettings{};
	TracePlotHandler::Settings traceSettings{};
	IDebugProbe::DebugProbeSettings debugProbeSettings{};
	ITraceProbe::TraceProbeSettings traceProbeSettings{};

	if (!file->read(*ini))
		return false;

	auto getValue = [&](std::string&& category, std::string&& field, auto&& result)
	{
		try
		{
			std::string value = ini->get(category).get(field);
			parseValue(value, result);
		}
		catch (const std::exception& ex)
		{
			logger->error("config parsing exception {}", ex.what());
		}
	};

	elfPath = ini->get("elf").get("file_path");

	getValue("settings", "version", globalSettings.version);
	getValue("settings", "sample_frequency_Hz", viewerSettings.sampleFrequencyHz);
	getValue("settings", "max_points", viewerSettings.maxPoints);
	getValue("settings", "max_viewport_points", viewerSettings.maxViewportPoints);
	getValue("settings", "stop_acq_on_elf_change", viewerSettings.stopAcqusitionOnElfChange);
	getValue("settings", "refresh_on_elf_change", viewerSettings.refreshAddressesOnElfChange);
	getValue("settings", "probe_type", debugProbeSettings.debugProbe);
	debugProbeSettings.device = ini->get("settings").get("target_name");
	getValue("settings", "probe_mode", debugProbeSettings.mode);
	getValue("settings", "probe_speed_kHz", debugProbeSettings.speedkHz);
	debugProbeSettings.serialNumber = ini->get("settings").get("probe_SN");
	getValue("settings", "should_log", viewerSettings.shouldLog);
	viewerSettings.logFilePath = ini->get("settings").get("log_directory");
	viewerSettings.gdbCommand = ini->get("settings").get("gdb_command");

	if (viewerSettings.gdbCommand.empty())
		viewerSettings.gdbCommand = "gdb";

	getValue("trace_settings", "core_frequency", traceSettings.coreFrequency);
	getValue("trace_settings", "trace_prescaler", traceSettings.tracePrescaler);
	getValue("trace_settings", "max_points", traceSettings.maxPoints);
	getValue("trace_settings", "max_viewport_points_percent", traceSettings.maxViewportPointsPercent);
	getValue("trace_settings", "trigger_channel", traceSettings.triggerChannel);
	getValue("trace_settings", "trigger_level", traceSettings.triggerLevel);
	getValue("trace_settings", "timeout", traceSettings.timeout);
	getValue("trace_settings", "probe_type", traceProbeSettings.debugProbe);
	traceProbeSettings.device = ini->get("trace_settings").get("target_name");
	getValue("trace_settings", "probe_speed_kHz", traceProbeSettings.speedkHz);
	traceProbeSettings.serialNumber = ini->get("trace_settings").get("probe_SN");
	getValue("trace_settings", "should_log", traceSettings.shouldLog);
	traceSettings.logFilePath = ini->get("trace_settings").get("log_directory");

	/* TODO magic numbers (lots of them)! */
	if (traceSettings.timeout == 0)
		traceSettings.timeout = 2;

	if (traceSettings.maxViewportPointsPercent == 0 && traceSettings.maxPoints == 0)
		traceSettings.triggerChannel = -1;

	if (traceSettings.maxViewportPointsPercent == 0)
		traceSettings.maxViewportPointsPercent = 50;

	if (traceSettings.maxPoints == 0)
		traceSettings.maxPoints = 10000;

	if (viewerSettings.sampleFrequencyHz == 0)
		viewerSettings.sampleFrequencyHz = 100;

	if (viewerSettings.maxPoints == 0)
		viewerSettings.maxPoints = 1000;

	if (viewerSettings.maxViewportPoints == 0)
		viewerSettings.maxViewportPoints = viewerSettings.maxPoints;

	if (debugProbeSettings.speedkHz == 0)
		debugProbeSettings.speedkHz = 100;

	loadVariables();
	loadPlots();
	loadTracePlots();
	loadPlotGroups();

	plotHandler->setSettings(viewerSettings);
	plotHandler->setProbeSettings(debugProbeSettings);

	tracePlotHandler->setSettings(traceSettings);
	tracePlotHandler->setProbeSettings(traceProbeSettings);

	return true;
}

bool ConfigHandler::saveConfigFile(const std::string& elfPath, const std::string& newSavePath)
{
	PlotHandler::Settings viewerSettings = plotHandler->getSettings();
	TracePlotHandler::Settings traceSettings = tracePlotHandler->getSettings();
	IDebugProbe::DebugProbeSettings debugProbeSettings = plotHandler->getProbeSettings();
	ITraceProbe::TraceProbeSettings traceProbeSettings = tracePlotHandler->getProbeSettings();

	(*ini).clear();

	(*ini)["elf"]["file_path"] = elfPath;

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	auto plotFieldFromID = [](uint32_t id, const std::string& prefix = "")
	{ return std::string(prefix + "plot" + std::to_string(id)); };

	auto groupFieldFromID = [](uint32_t id, const std::string& prefix = "")
	{ return std::string(prefix + "group" + std::to_string(id)); };

	auto plotGroupFieldFromID = [](uint32_t groupId, uint32_t plotId, const std::string& prefix = "")
	{ return std::string(prefix + "group" + std::to_string(groupId) + "-" + "plot" + std::to_string(plotId)); };

	auto plotSeriesFieldFromID = [](uint32_t plotId, uint32_t seriesId, const std::string& prefix = "")
	{ return std::string(prefix + "plot" + std::to_string(plotId) + "-" + "series" + std::to_string(seriesId)); };

	(*ini)["settings"]["version"] = std::to_string(globalSettings.version);

	(*ini)["settings"]["sample_frequency_hz"] = std::to_string(viewerSettings.sampleFrequencyHz);
	(*ini)["settings"]["max_points"] = std::to_string(viewerSettings.maxPoints);
	(*ini)["settings"]["max_viewport_points"] = std::to_string(viewerSettings.maxViewportPoints);
	(*ini)["settings"]["refresh_on_elf_change"] = viewerSettings.refreshAddressesOnElfChange ? std::string("true") : std::string("false");
	(*ini)["settings"]["stop_acq_on_elf_change"] = viewerSettings.stopAcqusitionOnElfChange ? std::string("true") : std::string("false");
	(*ini)["settings"]["probe_type"] = std::to_string(debugProbeSettings.debugProbe);
	(*ini)["settings"]["target_name"] = debugProbeSettings.device;
	(*ini)["settings"]["probe_mode"] = std::to_string(debugProbeSettings.mode);
	(*ini)["settings"]["probe_speed_kHz"] = std::to_string(debugProbeSettings.speedkHz);
	(*ini)["settings"]["probe_SN"] = debugProbeSettings.serialNumber;
	(*ini)["settings"]["should_log"] = viewerSettings.shouldLog ? std::string("true") : std::string("false");
	(*ini)["settings"]["log_directory"] = viewerSettings.logFilePath;
	(*ini)["settings"]["gdb_command"] = viewerSettings.gdbCommand;

	(*ini)["trace_settings"]["core_frequency"] = std::to_string(traceSettings.coreFrequency);
	(*ini)["trace_settings"]["trace_prescaler"] = std::to_string(traceSettings.tracePrescaler);
	(*ini)["trace_settings"]["max_points"] = std::to_string(traceSettings.maxPoints);
	(*ini)["trace_settings"]["max_viewport_points_percent"] = std::to_string(traceSettings.maxViewportPointsPercent);
	(*ini)["trace_settings"]["trigger_channel"] = std::to_string(traceSettings.triggerChannel);
	(*ini)["trace_settings"]["trigger_level"] = std::to_string(traceSettings.triggerLevel);
	(*ini)["trace_settings"]["timeout"] = std::to_string(traceSettings.timeout);
	(*ini)["trace_settings"]["probe_type"] = std::to_string(traceProbeSettings.debugProbe);
	(*ini)["trace_settings"]["target_name"] = traceProbeSettings.device;
	(*ini)["trace_settings"]["probe_speed_kHz"] = std::to_string(traceProbeSettings.speedkHz);
	(*ini)["trace_settings"]["probe_SN"] = traceProbeSettings.serialNumber;
	(*ini)["trace_settings"]["should_log"] = traceSettings.shouldLog ? std::string("true") : std::string("false");
	(*ini)["trace_settings"]["log_directory"] = traceSettings.logFilePath;

	uint32_t varId = 0;
	for (std::shared_ptr<Variable> var : *variableHandler)
	{
		(*ini)[varFieldFromID(varId)]["name"] = var->getName();
		(*ini)[varFieldFromID(varId)]["tracked_name"] = var->getTrackedName();
		(*ini)[varFieldFromID(varId)]["address"] = std::to_string(var->getAddress());
		(*ini)[varFieldFromID(varId)]["type"] = std::to_string(static_cast<uint8_t>(var->getType()));
		(*ini)[varFieldFromID(varId)]["color"] = std::to_string(static_cast<uint32_t>(var->getColorU32()));
		(*ini)[varFieldFromID(varId)]["should_update_from_elf"] = var->getShouldUpdateFromElf() ? "true" : "false";
		(*ini)[varFieldFromID(varId)]["shift"] = std::to_string(var->getShift());
		(*ini)[varFieldFromID(varId)]["mask"] = std::to_string(var->getMask());
		(*ini)[varFieldFromID(varId)]["high_level_type"] = std::to_string(static_cast<uint8_t>(var->getHighLevelType()));

		if (var->getHighLevelType() != Variable::HighLevelType::NONE)
		{
			auto fractional = var->getFractional();
			(*ini)[varFieldFromID(varId)]["frac"] = std::to_string(fractional.fractionalBits);
			(*ini)[varFieldFromID(varId)]["base"] = std::to_string(fractional.base);
		}

		varId++;
	}

	uint32_t plotId = 0;
	for (std::shared_ptr<Plot> plt : *plotHandler)
	{
		(*ini)[plotFieldFromID(plotId)]["name"] = plt->getName();
		(*ini)[plotFieldFromID(plotId)]["visibility"] = plt->getVisibility() ? "true" : "false";
		(*ini)[plotFieldFromID(plotId)]["type"] = std::to_string(static_cast<uint8_t>(plt->getType()));

		uint32_t serId = 0;

		for (auto& [key, ser] : plt->getSeriesMap())
		{
			(*ini)[plotSeriesFieldFromID(plotId, serId)]["name"] = ser->var->getName();
			(*ini)[plotSeriesFieldFromID(plotId, serId)]["visibility"] = ser->visible ? "true" : "false";

			std::string displayFormat = "DEC";
			for (auto [format, value] : displayFormatMap)
			{
				if (value == ser->format)
				{
					displayFormat = format;
					break;
				}
			}

			(*ini)[plotSeriesFieldFromID(plotId, serId)]["format"] = displayFormat;
			serId++;
		}

		plotId++;
	}

	uint32_t groupId = 0;
	for (auto& [name, group] : *plotGroupHandler)
	{
		(*ini)[groupFieldFromID(groupId)]["name"] = group->getName();

		uint32_t plotId = 0;
		for (auto& [name, plot] : *group)
		{
			(*ini)[plotGroupFieldFromID(groupId, plotId)]["name"] = plot->getName();
			plotId++;
		}
		groupId++;
	}

	plotId = 0;
	for (std::shared_ptr<Plot> plt : *tracePlotHandler)
	{
		const std::string plotName = plotFieldFromID(plotId, "trace_");

		(*ini)[plotName]["name"] = plt->getName();
		(*ini)[plotName]["alias"] = plt->getAlias();
		(*ini)[plotName]["visibility"] = plt->getVisibility() ? "true" : "false";
		(*ini)[plotName]["domain"] = std::to_string(static_cast<uint8_t>(plt->getDomain()));
		if (plt->getDomain() == Plot::Domain::ANALOG)
			(*ini)[plotName]["type"] = std::to_string(static_cast<uint8_t>(plt->getTraceVarType()));
		plotId++;
	}

	if (newSavePath != "")
	{
		file.reset();
		file = std::make_unique<mINI::INIFile>(newSavePath);
	}

	return file->generate(*ini, true);
}