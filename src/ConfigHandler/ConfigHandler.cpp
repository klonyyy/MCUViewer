#include "ConfigHandler.hpp"

#include <iostream>
#include <random>

ConfigHandler::ConfigHandler(std::string configFilePath, PlotHandler* plotHandler) : configFilePath(configFilePath), plotHandler(plotHandler)
{
	ini = std::make_unique<mINI::INIStructure>();
	file = std::make_unique<mINI::INIFile>(configFilePath);
}

bool ConfigHandler::changeConfigFile(std::string newConfigFilePath)
{
	configFilePath = newConfigFilePath;
	file.reset();
	file = std::make_unique<mINI::INIFile>(configFilePath);
	return true;
}
std::string ConfigHandler::getElfFilePath()
{
	return std::string(ini->get("elf").get("file_path"));
}

bool ConfigHandler::readConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, std::string& elfPath)
{
	if (!file->read(*ini))
		return false;

	uint32_t varId = 0;
	std::string name = "xxx";

	elfPath = ini->get("elf").get("file_path");

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

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
			std::cout << "ADDING VARIABLE: " << newVar->getName() << std::endl;
		}
	}

	std::string plotName("xxx");
	uint32_t plotNumber = 0;

	while (!plotName.empty())
	{
		std::string sectionName("plot" + std::to_string(plotNumber++));
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
			std::string varName = ini->get(sectionName).get(std::string("series" + std::to_string(seriesNumber++)));

			while (varName != "")
			{
				plotHandler->getPlot(plotName)->addSeries(*vars[varName]);
				std::cout << "ADDING SERIES: " << varName << std::endl;
				varName = ini->get(sectionName).get(std::string("series" + std::to_string(seriesNumber++)));
			}
		}
	}

	return true;
}

bool ConfigHandler::saveConfigFile(std::map<std::string, std::shared_ptr<Variable>>& vars, std::string& elfPath, std::string newSavePath)
{
	(*ini).clear();

	(*ini)["elf"]["file_path"] = elfPath;

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	auto plotFieldFromID = [](uint32_t id)
	{ return std::string("plot" + std::to_string(id)); };

	auto seriesFieldFromID = [](uint32_t id)
	{ return std::string("series" + std::to_string(id)); };

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
	for (Plot* plt : *plotHandler)
	{
		(*ini)[plotFieldFromID(plotId)]["name"] = plt->getName();
		(*ini)[plotFieldFromID(plotId)]["visibility"] = plt->getVisibility() ? "true" : "false";
		(*ini)[plotFieldFromID(plotId)]["type"] = std::to_string(static_cast<uint8_t>(plt->getType()));

		uint32_t serId = 0;

		for (auto& [key, ser] : plt->getSeriesMap())
			(*ini)[plotFieldFromID(plotId)][seriesFieldFromID(serId++)] = ser->var->getName();

		plotId++;
	}

	if (newSavePath != "")
	{
		file.reset();
		file = std::make_unique<mINI::INIFile>(newSavePath);
	}

	return file->generate(*ini, true);
}