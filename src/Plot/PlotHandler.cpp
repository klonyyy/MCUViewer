#include "PlotHandler.hpp"

#include <algorithm>
#include <array>

PlotHandler::PlotHandler(bool& done, std::mutex* mtx) : done(done), mtx(mtx)
{
	dataHandle = std::thread(&PlotHandler::dataHandler, this);
	vals = new VarReader();
}
PlotHandler::~PlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
	delete vals;
}

void PlotHandler::addPlot(std::string name)
{
	plotsMap[name] = new Plot(name);
}
bool PlotHandler::removePlot(std::string name)
{
	delete plotsMap[name];
	plotsMap.erase(name);
	return true;
}

bool PlotHandler::renamePlot(std::string oldName, std::string newName)
{
	auto plt = plotsMap.extract(oldName);
	plt.key() = newName;
	plotsMap.insert(std::move(plt));
	plotsMap[newName]->setName(newName);
	return true;
}

bool PlotHandler::removeAllPlots()
{
	for (auto& plot : plotsMap)
		removePlot(plot.first);
	return true;
}

Plot* PlotHandler::getPlot(std::string name)
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

bool PlotHandler::getViewerState()
{
	return static_cast<bool>(viewerState);
}

uint32_t PlotHandler::getVisiblePlotsCount()
{
	return std::count_if(plotsMap.begin(), plotsMap.end(), [](const auto& pair)
						 { return pair.second->getVisibility(); });
}
uint32_t PlotHandler::getPlotsCount()
{
	return plotsMap.size();
}

void PlotHandler::dataHandler()
{
	while (!done)
	{
		if (viewerState == state::RUN)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(10));
			auto finish = std::chrono::steady_clock::now();
			double t = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();

			for (auto& [key, plot] : plotsMap)
			{
				int i = 0;
				/* this part consumes most of the thread time */
				std::array<float, maxVariables> values;
				for (auto& [name, ser] : plot->getSeriesMap())
					values[i++] = vals->getFloat(ser->var->getAddress(), ser->var->getType());

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
				vals->start();
			}
			else
				vals->stop();
			stateChangeOrdered = false;
		}
	}
}
bool PlotHandler::writeSeriesValue(Variable& var, float value)
{
	std::lock_guard<std::mutex> lock(*mtx);
	return vals->setValue(var, value);
}