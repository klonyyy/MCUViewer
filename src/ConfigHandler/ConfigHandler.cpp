#include "ConfigHandler.hpp"

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

bool ConfigHandler::readConfigFile(std::vector<Variable>& vars, std::string& elfPath)
{
	if (!file->read(*ini))
		return false;

	uint32_t varId = 0;
	Variable newVar("xxx");

	elfPath = ini->get("elf").get("file_path");

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	while (!newVar.getName().empty())
	{
		newVar.setName(ini->get(varFieldFromID(varId)).get("name"));
		newVar.setAddress(atoi(ini->get(varFieldFromID(varId)).get("address").c_str()));
		newVar.setType(static_cast<Variable::type>(atoi(ini->get(varFieldFromID(varId)).get("type").c_str())));
		newVar.setColor(static_cast<uint32_t>(atol(ini->get(varFieldFromID(varId)).get("color").c_str())));
		varId++;

		if (!newVar.getName().empty())
		{
			vars.push_back(newVar);
			std::cout << "ADDING VARIABLE: " << newVar.getName() << std::endl;
		}
	}

	std::string plotName("xxx");
	uint32_t plotNumber = 0;

	while (!plotName.empty())
	{
		std::string sectionName("plot" + std::to_string(plotNumber++));
		plotName = ini->get(sectionName).get("name");

		if (!plotName.empty())
		{
			uint32_t plotId = plotHandler->addPlot(plotName);

			std::cout << "ADDING PLOT: " << plotName << std::endl;

			uint32_t seriesNumber = 0;
			std::string varName = "xxx";
			while (varName != "")
			{
				varName = ini->get(sectionName).get(std::string("series" + std::to_string(seriesNumber++)));

				for (auto& var : vars)
				{
					if (varName == var.getName())
					{
						plotHandler->getPlot(plotId)->addSeries(var);
						std::cout << "ADDING SERIES: " << var.getName() << std::endl;
					}
				}
			}
		}
	}

	return true;
}

bool ConfigHandler::saveConfigFile(std::vector<Variable>& vars, std::string& elfPath, std::string newSavePath)
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
	for (auto& var : vars)
	{
		(*ini)[varFieldFromID(varId)]["name"] = var.getName();
		(*ini)[varFieldFromID(varId)]["address"] = std::to_string(var.getAddress());
		(*ini)[varFieldFromID(varId)]["type"] = std::to_string(static_cast<uint8_t>(var.getType()));
		(*ini)[varFieldFromID(varId)]["color"] = std::to_string(static_cast<uint32_t>(var.getColorU32()));

		varId++;
	}

	for (uint32_t plotId = 0; plotId < plotHandler->getPlotsCount(); plotId++)
	{
		Plot* plt = plotHandler->getPlot(plotId);
		(*ini)[plotFieldFromID(plotId)]["name"] = plt->getName();
		(*ini)[plotFieldFromID(plotId)]["visibility"] = plt->getVisibility() ? "true" : "false";

		uint32_t serId = 0;

		for (auto& [key, value] : plt->getSeriesMap())
		{
			(*ini)[plotFieldFromID(plotId)][seriesFieldFromID(serId++)] = *value.get()->seriesName;
		}
	}

	if (newSavePath != "")
	{
		file.reset();
		file = std::make_unique<mINI::INIFile>(newSavePath);
	}

	return file->generate(*ini, true);
}