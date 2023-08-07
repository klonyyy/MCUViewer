#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#include "Plot.hpp"
#include "PlotHandlerBase.hpp"
#include "ScrollingBuffer.hpp"
#include "StlinkHandler.hpp"
#include "TargetMemoryHandler.hpp"
#include "spdlog/spdlog.h"

class PlotHandler : public PlotHandlerBase
{
   public:
	PlotHandler(bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
	virtual ~PlotHandler();

	std::string getLastReaderError() const;
	bool writeSeriesValue(Variable& var, double value);
	void setSamplePeriod(uint32_t period);
	uint32_t getSamplePeriod() const;

   private:
	void dataHandler();

   private:
	std::unique_ptr<StlinkHandler> stlinkReader;
	std::unique_ptr<TargetMemoryHandler> varReader;
	std::chrono::time_point<std::chrono::steady_clock> start;
	float samplePeriodMs = 1;
};

#endif