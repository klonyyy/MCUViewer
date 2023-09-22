#ifndef _TRACEPLOTHANDLER_HPP
#define _TRACEPLOTHANDLER_HPP

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Plot.hpp"
#include "PlotHandlerBase.hpp"
#include "StlinkTraceDevice.hpp"
#include "TraceReader.hpp"
#include "spdlog/spdlog.h"

class TracePlotHandler : public PlotHandlerBase
{
   public:
	typedef struct
	{
		uint32_t coreFrequency = 160000;
		uint32_t tracePrescaler = 10;
		uint32_t maxPoints = 10000;
		uint32_t maxViewportPointsPercent = 10;
		int32_t triggerChannel = -1;
		double triggerLevel = 0.9;
	} Settings;

	TracePlotHandler(std::atomic<bool>& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
	~TracePlotHandler();

	void initPlots();

	Settings getSettings() const;
	void setSettings(const Settings& settings);

	TraceReader::TraceIndicators getTraceIndicators() const;
	std::vector<double> getErrorTimestamps();
	std::string getLastReaderError() const;

	void setTriggerChannel(int32_t triggerChannel);
	int32_t getTriggerChannel() const;
	double getDoubleValue(const Plot& plot, uint32_t value);
	std::map<std::string, std::shared_ptr<Variable>> traceVars;

   private:
	void dataHandler();

   private:
	Settings traceSettings{};
	std::shared_ptr<StlinkTraceDevice> traceDevice;
	std::unique_ptr<TraceReader> traceReader;

	std::deque<double> errorFrameTimestamps{};
	uint32_t errorFrameSinceLastPoint = 0;
	std::string lastErrorMsg{};

	bool traceTriggered = false;
	static constexpr uint32_t channels = 10;
	static constexpr size_t maxAllowedViewportErrors = 100;
	double time = 0.0;
};

#endif