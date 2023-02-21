#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#include "Plot.hpp"
#include "ScrollingBuffer.hpp"
#include "VarReader.hpp"

class PlotHandler
{
   public:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};

	PlotHandler()
	{
		dataHandle = std::thread(&PlotHandler::dataHandler, this);
		vals = new VarReader();
	}
	~PlotHandler()
	{
		if (dataHandle.joinable())
			dataHandle.join();
		delete vals;
	}

	uint32_t addPlot(std::string name)
	{
		uint32_t newId = plotsMap.size();
		plotsMap[newId] = new Plot(name);
		return newId;
	}
	bool removePlot(uint32_t id)
	{
		delete plotsMap[id];
		plotsMap.erase(id);
		return true;
	}

	bool removeAllPlots()
	{
		for (auto& plot : plotsMap)
			removePlot(plot.first);
		return true;
	}

	Plot* getPlot(uint32_t id)
	{
		return plotsMap[id];
	}

	uint32_t getPlotsCount()
	{
		return plotsMap.size();
	}

	bool drawAll()
	{
		if (plotsMap.empty())
			return false;

		for (auto& plot : plotsMap)
			if (plot.second != nullptr)
				plot.second->draw();

		return false;
	}

	bool eraseAllPlotData()
	{
		if (plotsMap.empty())
			return false;

		for (auto& plot : plotsMap)
			if (plot.second != nullptr)
				plot.second->erase();

		return false;
	}

	void setViewerState(state state)
	{
		if (state == viewerState)
			return;
		stateChangeOrdered = true;
		viewerStateTemp = state;
	}

   private:
	state viewerState = state::STOP;
	state viewerStateTemp = state::STOP;
	VarReader* vals;

	std::map<uint32_t, Plot*> plotsMap;

	std::mutex mtx;
	std::thread dataHandle;

	bool stateChangeOrdered = false;

	std::chrono::time_point<std::chrono::steady_clock> start;

	void dataHandler()
	{
		while (1)
		{
			if (viewerState == state::RUN)
			{
				mtx.lock();
				std::this_thread::sleep_for(std::chrono::microseconds(10));
				auto finish = std::chrono::steady_clock::now();
				double t = std::chrono::duration_cast<
							   std::chrono::duration<double> >(finish - start)
							   .count();

				for (auto& plot : plotsMap)
				{
					std::vector<uint32_t> addresses = plot.second->getVariableAddesses();
					std::vector<Variable::type> types = plot.second->getVariableTypes();
					int i = 0;
					for (auto& adr : addresses)
					{
						plot.second->addPoint(t, adr, vals->getFloat(adr, types[i++]));
					}

					plot.second->addTimePoint(t);
				}
				mtx.unlock();
			}
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
};

#endif