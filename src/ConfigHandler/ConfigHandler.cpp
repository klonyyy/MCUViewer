#include "ConfigHandler.hpp"

#include <iostream>
#include <random>

ConfigHandler::ConfigHandler(const std::string& configFilePath, PlotHandler* plotHandler) : configFilePath(configFilePath), plotHandler(plotHandler)
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

bool ConfigHandler::readConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, std::string& elfPath, Settings& settings) const
{
	if (!file->read(*ini))
		return false;

	uint32_t varId = 0;
	std::string name = "xxx";

	elfPath = ini->get("elf").get("file_path");

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	settings.version = atoi(ini->get("settings").get("version").c_str());
	settings.samplePeriod = atoi(ini->get("settings").get("sample_period").c_str());
	settings.maxPoints = atoi(ini->get("settings").get("max_points").c_str());
	settings.maxViewportPoints = atoi(ini->get("settings").get("max_viewport_points").c_str());

	if (settings.samplePeriod == 0)
		settings.samplePeriod = 10;

	if (settings.maxPoints == 0)
		settings.maxPoints = 1000;

	if (settings.maxViewportPoints == 0)
		settings.maxViewportPoints = settings.maxPoints;

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
			std::cout << "--------- WARNING: unaligned variable address! ----------" << std::endl;

		if (!newVar->getName().empty())
		{
			vars[newVar->getName()] = newVar;
			newVar->setIsFound(true);
			std::cout << "ADDING VARIABLE: " << newVar->getName() << std::endl;
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
		Plot::type_E type = static_cast<Plot::type_E>(atoi(ini->get(sectionName).get("type").c_str()));

		if (!plotName.empty())
		{
			plotHandler->addPlot(plotName);
			plotHandler->getPlot(plotName)->setVisibility(visibility);
			plotHandler->getPlot(plotName)->setType(type);

			std::cout << "ADDING PLOT: " << plotName << std::endl;
			uint32_t seriesNumber = 0;
			std::string varName = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("name");

			while (varName != "")
			{
				plotHandler->getPlot(plotName)->addSeries(*vars[varName]);
				bool visible = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("visibility") == "true" ? true : false;
				plotHandler->getPlot(plotName)->getSeries(varName)->visible = visible;
				std::string displayFormat = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("format");
				if (displayFormat == "")
					displayFormat = "DEC";
				plotHandler->getPlot(plotName)->getSeries(varName)->format = displayFormatMap.at(displayFormat);
				std::cout << "ADDING SERIES: " << varName << std::endl;
				seriesNumber++;
				varName = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("name");
			}
		}
		plotNumber++;
	}

	return true;
}

bool ConfigHandler::saveConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, const std::string& elfPath, const Settings& settings, const std::string newSavePath)
{
	(*ini).clear();

	(*ini)["elf"]["file_path"] = elfPath;

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	auto plotFieldFromID = [](uint32_t id)
	{ return std::string("plot" + std::to_string(id)); };

	auto plotSeriesFieldFromID = [](uint32_t plotId, uint32_t seriesId)
	{ return std::string("plot" + std::to_string(plotId) + "-" + "series" + std::to_string(seriesId)); };

	(*ini)["settings"]["sample_period"] = std::to_string(settings.samplePeriod);
	(*ini)["settings"]["version"] = std::to_string(settings.version);
	(*ini)["settings"]["max_points"] = std::to_string(settings.maxPoints);
	(*ini)["settings"]["max_viewport_points"] = std::to_string(settings.maxViewportPoints);

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
			for (auto [key, value] : displayFormatMap)
			{
				if (value == ser->format)
				{
					displayFormat = key;
					break;
				}
			}

			(*ini)[plotSeriesFieldFromID(plotId, serId)]["format"] = displayFormat;
			serId++;
		}

		plotId++;
	}

	if (newSavePath != "")
	{
		file.reset();
		file = std::make_unique<mINI::INIFile>(newSavePath);
	}

	return file->generate(*ini, true);
}