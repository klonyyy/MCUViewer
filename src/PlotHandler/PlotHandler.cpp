#include "PlotHandler.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>


PlotHandler::Settings PlotHandler::getSettings() const
{
	return settings;
}

void PlotHandler::setSettings(const Settings& newSettings)
{
	settings = newSettings;
	setMaxPoints(settings.maxPoints);
}



