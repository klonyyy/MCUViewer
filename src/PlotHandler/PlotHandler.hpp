#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#include "Plot.hpp"
#include "PlotHandlerBase.hpp"
#include "ScrollingBuffer.hpp"
//#include "StlinkHandler.hpp"
#include "JlinkHandler.h"
#include "TargetMemoryHandler.hpp"
#include "spdlog/spdlog.h"

class PlotHandler : public PlotHandlerBase
{
   public:
	typedef struct Settings
	{
		uint32_t samplePeriod = 10;
		uint32_t maxPoints = 10000;
		uint32_t maxViewportPoints = 5000;
	} Settings;

	PlotHandler(std::atomic<bool>& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
	virtual ~PlotHandler();

	std::string getLastReaderError() const;
	bool writeSeriesValue(Variable& var, double value);

	Settings getSettings() const;
	void setSettings(Settings& newSettings);

   private:
	void dataHandler();

   private:
	std::unique_ptr<JlinkHandler> stlinkReader;
	std::unique_ptr<TargetMemoryHandler> varReader;
	std::chrono::time_point<std::chrono::steady_clock> start;
	Settings settings{};
	// float samplePeriodMs = 1;
};

#endif