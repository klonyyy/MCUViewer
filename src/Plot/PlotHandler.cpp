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