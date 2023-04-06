#include "PlotHandler.hpp"

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

uint32_t PlotHandler::addPlot(std::string name)
{
	uint32_t newId = plotsMap.size();
	plotsMap[newId] = new Plot(name);
	return newId;
}
bool PlotHandler::removePlot(uint32_t id)
{
	delete plotsMap[id];
	plotsMap.erase(id);
	return true;
}

bool PlotHandler::removeAllPlots()
{
	for (auto& plot : plotsMap)
		removePlot(plot.first);
	return true;
}

Plot* PlotHandler::getPlot(uint32_t id)
{
	return plotsMap[id];
}

uint32_t PlotHandler::getPlotsCount()
{
	return plotsMap.size();
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
				std::vector<uint32_t> addresses = plot->getVariableAddesses();
				std::vector<Variable::type> types = plot->getVariableTypes();
				int i = 0;

				/* this part consumes most of the thread time */
				std::array<float, maxVariables> values;
				for (auto& adr : addresses)
				{
					values[i] = vals->getFloat(adr, types[i]);
					i++;
				}

				/* thread-safe part */
				std::lock_guard<std::mutex> lock(*mtx);
				i = 0;
				for (auto& adr : addresses)
					plot->addPoint(adr, values[i++]);
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