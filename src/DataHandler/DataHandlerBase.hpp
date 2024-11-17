#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include "CSVStreamer.hpp"
#include "PlotGroupHandler.hpp"
#include "PlotHandler.hpp"
#include "VariableHandler.hpp"
#include "spdlog/spdlog.h"

class DataHandlerBase
{
   public:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};

	DataHandlerBase(PlotGroupHandler* plotGroupHandler, VariableHandler* variableHandler, PlotHandler* plotHandler, PlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger) : plotGroupHandler(plotGroupHandler), variableHandler(variableHandler), plotHandler(plotHandler), tracePlotHandler(tracePlotHandler), done(done), mtx(mtx), logger(logger)
	{
		csvStreamer = std::make_unique<CSVStreamer>(logger);
	}
	virtual ~DataHandlerBase() = default;

	virtual std::string getLastReaderError() const = 0;

	void setState(state state)
	{
		if (state == viewerState)
			return;

		viewerState = state;
		stateChangeOrdered = true;
	}
	state getState() const
	{
		/* TODO possible deadlock */
		while (stateChangeOrdered);
		return viewerState;
	}

   protected:
	PlotGroupHandler* plotGroupHandler;
	VariableHandler* variableHandler;
	PlotHandler* plotHandler;
	PlotHandler* tracePlotHandler;
	std::atomic<bool>& done;
	std::atomic<state> viewerState = state::STOP;
	std::mutex* mtx;
	std::thread dataHandle;
	std::atomic<bool> stateChangeOrdered = false;
	spdlog::logger* logger;

	std::unique_ptr<CSVStreamer> csvStreamer;
};