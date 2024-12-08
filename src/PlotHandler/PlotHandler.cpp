#include "PlotHandler.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

std::shared_ptr<Plot> PlotHandler::addPlot(const std::string& name)
{
	plotsMap[name] = std::make_shared<Plot>(name);
	return plotsMap[name];
}

bool PlotHandler::removePlot(const std::string& name)
{
	plotsMap.erase(name);
	return true;
}

bool PlotHandler::renamePlot(const std::string& oldName, const std::string& newName)
{
	auto plt = plotsMap.extract(oldName);
	plt.key() = newName;
	plotsMap.insert(std::move(plt));
	plotsMap[newName]->setName(newName);
	return true;
}

bool PlotHandler::removeAllPlots()
{
	plotsMap.clear();
	return true;
}

std::shared_ptr<Plot> PlotHandler::getPlot(std::string name)
{
	return plotsMap.at(name);
}

bool PlotHandler::eraseAllPlotData()
{
	if (plotsMap.empty())
		return false;

	for (auto& plot : plotsMap)
		if (plot.second != nullptr)
			plot.second->erase();

	return true;
}

uint32_t PlotHandler::getVisiblePlotsCount() const
{
	return std::count_if(plotsMap.begin(), plotsMap.end(), [](const auto& pair)
						 { return pair.second->getVisibility(); });
}

uint32_t PlotHandler::getPlotsCount() const
{
	return plotsMap.size();
}

bool PlotHandler::checkIfPlotExists(const std::string& name) const
{
	return plotsMap.find(name) != plotsMap.end();
}

void PlotHandler::setMaxPoints(uint32_t maxPoints)
{
	if (maxPoints == 0)
		return;

	for (auto& [name, plt] : plotsMap)
	{
		for (auto& [serName, ser] : plt->getSeriesMap())
			ser->buffer->setMaxSize(maxPoints);
		plt->getXAxisSeries()->setMaxSize(maxPoints);
	}
}

PlotHandler::iterator::iterator(std::map<std::string, std::shared_ptr<Plot>>::iterator iter)
	: m_iter(iter)
{
}

PlotHandler::iterator& PlotHandler::iterator::operator++()
{
	++m_iter;
	return *this;
}

PlotHandler::iterator PlotHandler::iterator::operator++(int)
{
	iterator tmp = *this;
	++(*this);
	return tmp;
}

bool PlotHandler::iterator::operator==(const iterator& other) const
{
	return m_iter == other.m_iter;
}

bool PlotHandler::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}

std::shared_ptr<Plot> PlotHandler::iterator::operator*()
{
	return m_iter->second;
}

PlotHandler::iterator PlotHandler::begin()
{
	return iterator(plotsMap.begin());
}

PlotHandler::iterator PlotHandler::end()
{
	return iterator(plotsMap.end());
}
