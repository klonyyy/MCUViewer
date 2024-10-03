#include "PlotHandlerBase.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

PlotHandlerBase::PlotHandlerBase(std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger) : done(done), mtx(mtx), logger(logger)
{
	csvStreamer = std::make_unique<CSVStreamer>(logger);
}

void PlotHandlerBase::addPlot(const std::string& name)
{
	plotsMap[name] = std::make_shared<Plot>(name);
}

bool PlotHandlerBase::removePlot(const std::string& name)
{
	plotsMap.erase(name);
	return true;
}

bool PlotHandlerBase::renamePlot(const std::string& oldName, const std::string& newName)
{
	auto plt = plotsMap.extract(oldName);
	plt.key() = newName;
	plotsMap.insert(std::move(plt));
	plotsMap[newName]->setName(newName);
	return true;
}

bool PlotHandlerBase::removeAllPlots()
{
	plotsMap.clear();
	return true;
}

std::shared_ptr<Plot> PlotHandlerBase::getPlot(std::string name)
{
	return plotsMap.at(name);
}

bool PlotHandlerBase::eraseAllPlotData()
{
	if (plotsMap.empty())
		return false;

	for (auto& plot : plotsMap)
		if (plot.second != nullptr)
			plot.second->erase();

	return true;
}

void PlotHandlerBase::setViewerState(state state)
{
	if (state == viewerState)
		return;

	viewerState = state;
	stateChangeOrdered = true;
}

PlotHandlerBase::state PlotHandlerBase::getViewerState() const
{
	/* TODO possible deadlock */
	while (stateChangeOrdered)
	{
	}
	return viewerState;
}

uint32_t PlotHandlerBase::getVisiblePlotsCount() const
{
	return std::count_if(plotsMap.begin(), plotsMap.end(), [](const auto& pair)
						 { return pair.second->getVisibility(); });
}
uint32_t PlotHandlerBase::getPlotsCount() const
{
	return plotsMap.size();
}

bool PlotHandlerBase::checkIfPlotExists(const std::string&& name) const
{
	return plotsMap.find(name) != plotsMap.end();
}

void PlotHandlerBase::setMaxPoints(uint32_t maxPoints)
{
	if (maxPoints == 0)
		return;

	for (auto& [name, plt] : plotsMap)
	{
		for (auto& [serName, ser] : plt->getSeriesMap())
			ser->buffer->setMaxSize(maxPoints);
		plt->getTimeSeries().setMaxSize(maxPoints);
	}
}

PlotHandlerBase::iterator::iterator(std::map<std::string, std::shared_ptr<Plot>>::iterator iter)
	: m_iter(iter)
{
}

PlotHandlerBase::iterator& PlotHandlerBase::iterator::operator++()
{
	++m_iter;
	return *this;
}

PlotHandlerBase::iterator PlotHandlerBase::iterator::operator++(int)
{
	iterator tmp = *this;
	++(*this);
	return tmp;
}

bool PlotHandlerBase::iterator::operator==(const iterator& other) const
{
	return m_iter == other.m_iter;
}

bool PlotHandlerBase::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}

std::shared_ptr<Plot> PlotHandlerBase::iterator::operator*()
{
	return m_iter->second;
}

PlotHandlerBase::iterator PlotHandlerBase::begin()
{
	return iterator(plotsMap.begin());
}

PlotHandlerBase::iterator PlotHandlerBase::end()
{
	return iterator(plotsMap.end());
}
