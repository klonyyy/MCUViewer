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

	PlotHandler();
	~PlotHandler();

	uint32_t addPlot(std::string name);
	bool removePlot(uint32_t id);
	bool removeAllPlots();
	Plot* getPlot(uint32_t id);
	uint32_t getPlotsCount();
	bool drawAll();
	bool eraseAllPlotData();
	void setViewerState(state state);

   private:
	state viewerState = state::STOP;
	state viewerStateTemp = state::STOP;
	VarReader* vals;

	std::map<uint32_t, Plot*> plotsMap;

	std::mutex mtx;
	std::thread dataHandle;

	bool stateChangeOrdered = false;

	std::chrono::time_point<std::chrono::steady_clock> start;

	void dataHandler();
};

#endif