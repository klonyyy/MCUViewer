#ifndef _CONFIGHANDLER_HPP
#define _CONFIGHANDLER_HPP

#include <string>

#include "PlotHandler.hpp"
#include "Variable.hpp"
#include "ini.h"

class ConfigHandler
{
   public:
	ConfigHandler(std::string configFilePath, PlotHandler* plotHandler) : plotHandler(plotHandler)
	{
		file = new mINI::INIFile(configFilePath);
		ini = new mINI::INIStructure();
	}
	~ConfigHandler() = default;

	bool readConfigFile(std::vector<Variable>& vars)
	{
		if (!file->read(*ini))
			return false;

		uint32_t varId = 0;
		Variable newVar("xxx");

		while (!newVar.getName().empty())
		{
			newVar.setName(ini->get(std::string("var" + std::to_string(varId))).get("name"));
			newVar.setAddress(atoi(ini->get(std::string("var" + std::to_string(varId))).get("address").c_str()));
			newVar.setType(static_cast<Variable::type>(atoi(ini->get(std::string("var" + std::to_string(varId))).get("type").c_str())));

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

				for (auto& var : vars)
				{
					std::string varIdStr = ini->get(sectionName).get(std::string("series" + std::to_string(seriesNumber++)));

					if (!varIdStr.empty())
					{
						uint32_t varId = atoi(varIdStr.c_str());
						plotHandler->getPlot(plotId)->addSeries(vars[varId]);
						std::cout << "ADDING SERIES: " << vars[varId].getName() << std::endl;
					}
				}
			}
		}

		return true;
	}

   private:
	PlotHandler* plotHandler;
	std::string configFilePath;
	mINI::INIFile* file;
	mINI::INIStructure* ini;
};

#endif