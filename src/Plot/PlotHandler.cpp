#include "PlotHandler.hpp"

#include <algorithm>
#include <array>

PlotHandler::PlotHandler(bool& done, std::mutex* mtx) : done(done), mtx(mtx)
{
	dataHandle = std::thread(&PlotHandler::dataHandler, this);
	varReader = std::make_unique<VarReader>();
}
PlotHandler::~PlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
}

void PlotHandler::addPlot(const std::string& name)
{
	plotsMap[name] = std::make_shared<Plot>(name);
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
	return plotsMap[name];
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

void PlotHandler::setViewerState(state state)
{
	if (state == viewerState)
		return;
	stateChangeOrdered = true;
	viewerStateTemp = state;
}

bool PlotHandler::getViewerState() const
{
	return static_cast<bool>(viewerState);
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

void PlotHandler::dataHandler()
{
	while (!done)
	{
		if (viewerState == state::RUN)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			auto finish = std::chrono::steady_clock::now();
			double t = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();

			for (auto& [key, plot] : plotsMap)
			{
				if (!plot->getVisibility())
					continue;

				int i = 0;
				/* this part consumes most of the thread time */
				std::array<float, maxVariables> values;
				for (auto& [name, ser] : plot->getSeriesMap())
					values[i++] = varReader->getFloat(ser->var->getAddress(), ser->var->getType());

				/* thread-safe part */
				std::lock_guard<std::mutex> lock(*mtx);
				i = 0;
				for (auto& [name, ser] : plot->getSeriesMap())
					plot->addPoint(name, values[i++]);
				plot->addTimePoint(t);
			}
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if (stateChangeOrdered)
		{
			viewerState = viewerStateTemp;

			if (viewerState == state::RUN)
			{
				start = std::chrono::steady_clock::now();
				varReader->start();
			}
			else
				varReader->stop();
			stateChangeOrdered = false;
		}
	}
}

bool PlotHandler::writeSeriesValue(Variable& var, float value)
{
	std::lock_guard<std::mutex> lock(*mtx);
	return varReader->setValue(var, value);
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