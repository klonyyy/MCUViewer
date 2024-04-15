#include "ConfigHandler.hpp"

#include <memory>
#include <random>
#include <variant>

/* TODO refactor whole config and persistent storage handling */
ConfigHandler::ConfigHandler(const std::string& configFilePath, PlotHandler* plotHandler, TracePlotHandler* tracePlotHandler, spdlog::logger* logger) : configFilePath(configFilePath), plotHandler(plotHandler), tracePlotHandler(tracePlotHandler), logger(logger)
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

bool ConfigHandler::readConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, std::string& elfPath)
{
	PlotHandler::Settings viewerSettings{};
	TracePlotHandler::Settings traceSettings{};
	PlotHandler::DebugProbeSettings& probeSettings = plotHandler->probeSettings;

	if (!file->read(*ini))
		return false;

	uint32_t varId = 0;
	std::string name = "xxx";

	elfPath = ini->get("elf").get("file_path");

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

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

	getValue("settings", "version", globalSettings.version);
	getValue("settings", "sample_frequency_Hz", viewerSettings.sampleFrequencyHz);
	getValue("settings", "max_points", viewerSettings.maxPoints);
	getValue("settings", "max_viewport_points", viewerSettings.maxViewportPoints);
	getValue("settings", "probe_type", probeSettings.debugProbe);
	probeSettings.device = ini->get("settings").get("target_name");

	getValue("trace_settings", "core_frequency", traceSettings.coreFrequency);
	getValue("trace_settings", "trace_prescaler", traceSettings.tracePrescaler);
	getValue("trace_settings", "max_points", traceSettings.maxPoints);
	getValue("trace_settings", "max_viewport_points_percent", traceSettings.maxViewportPointsPercent);
	getValue("trace_settings", "trigger_channel", traceSettings.triggerChannel);
	getValue("trace_settings", "trigger_level", traceSettings.triggerLevel);
	getValue("trace_settings", "timeout", traceSettings.timeout);

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

	while (!name.empty())
	{
		name = ini->get(varFieldFromID(varId)).get("name");

		std::shared_ptr<Variable> newVar = std::make_shared<Variable>("");

		newVar->setName(name);
		newVar->setAddress(atoi(ini->get(varFieldFromID(varId)).get("address").c_str()));
		newVar->setType(static_cast<Variable::type>(atoi(ini->get(varFieldFromID(varId)).get("type").c_str())));
		newVar->setColor(static_cast<uint32_t>(atol(ini->get(varFieldFromID(varId)).get("color").c_str())));
		varId++;

		if (newVar->getAddress() % 4 != 0)
			logger->warn("--------- Unaligned variable address! ----------");

		if (!newVar->getName().empty())
		{
			vars[newVar->getName()] = newVar;
			newVar->setIsFound(true);
			logger->info("Adding variable: {}", newVar->getName());
		}
	}

	auto plotSeriesFieldFromID = [](uint32_t plotId, uint32_t seriesId)
	{ return std::string("plot" + std::to_string(plotId) + "-" + "series" + std::to_string(seriesId)); };

	std::string plotName("xxx");
	uint32_t plotNumber = 0;

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
				plot->addSeries(*vars[varName]);
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

	plotName = "xxx";
	plotNumber = 0;
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
			plot->addSeries(*newVar);
			plot->getSeries(plotName)->visible = true;
		}
		plotNumber++;
	}

	tracePlotHandler->setSettings(traceSettings);
	plotHandler->setSettings(viewerSettings);

	return true;
}

bool ConfigHandler::saveConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, const std::string& elfPath, const std::string& newSavePath)
{
	PlotHandler::Settings viewerSettings = plotHandler->getSettings();
	TracePlotHandler::Settings traceSettings = tracePlotHandler->getSettings();
	PlotHandler::DebugProbeSettings& probeSettings = plotHandler->probeSettings;

	(*ini).clear();

	(*ini)["elf"]["file_path"] = elfPath;

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	auto plotFieldFromID = [](uint32_t id, const std::string& prefix = "")
	{ return std::string(prefix + "plot" + std::to_string(id)); };

	auto plotSeriesFieldFromID = [](uint32_t plotId, uint32_t seriesId, const std::string& prefix = "")
	{ return std::string(prefix + "plot" + std::to_string(plotId) + "-" + "series" + std::to_string(seriesId)); };

	(*ini)["settings"]["version"] = std::to_string(globalSettings.version);

	(*ini)["settings"]["sample_frequency_hz"] = std::to_string(viewerSettings.sampleFrequencyHz);
	(*ini)["settings"]["max_points"] = std::to_string(viewerSettings.maxPoints);
	(*ini)["settings"]["max_viewport_points"] = std::to_string(viewerSettings.maxViewportPoints);
	(*ini)["settings"]["probe_type"] = std::to_string(probeSettings.debugProbe);
	(*ini)["settings"]["target_name"] = probeSettings.device;

	(*ini)["trace_settings"]["core_frequency"] = std::to_string(traceSettings.coreFrequency);
	(*ini)["trace_settings"]["trace_prescaler"] = std::to_string(traceSettings.tracePrescaler);
	(*ini)["trace_settings"]["max_points"] = std::to_string(traceSettings.maxPoints);
	(*ini)["trace_settings"]["max_viewport_points_percent"] = std::to_string(traceSettings.maxViewportPointsPercent);
	(*ini)["trace_settings"]["trigger_channel"] = std::to_string(traceSettings.triggerChannel);
	(*ini)["trace_settings"]["trigger_level"] = std::to_string(traceSettings.triggerLevel);
	(*ini)["trace_settings"]["timeout"] = std::to_string(traceSettings.timeout);

	uint32_t varId = 0;
	for (auto& [key, var] : vars)
	{
		(*ini)[varFieldFromID(varId)]["name"] = var->getName();
		(*ini)[varFieldFromID(varId)]["address"] = std::to_string(var->getAddress());
		(*ini)[varFieldFromID(varId)]["type"] = std::to_string(static_cast<uint8_t>(var->getType()));
		(*ini)[varFieldFromID(varId)]["color"] = std::to_string(static_cast<uint32_t>(var->getColorU32()));

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