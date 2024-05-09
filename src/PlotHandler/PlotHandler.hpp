#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IDebugProbe.hpp"
#include "MovingAverage.hpp"
#include "Plot.hpp"
#include "PlotHandlerBase.hpp"
#include "ScrollingBuffer.hpp"
#include "TargetMemoryHandler.hpp"
#include "spdlog/spdlog.h"

class PlotHandler : public PlotHandlerBase
{
   public:
	typedef struct Settings
	{
		uint32_t sampleFrequencyHz = 100;
		uint32_t maxPoints = 10000;
		uint32_t maxViewportPoints = 5000;
		bool refreshAddressesOnElfChange = false;
		bool stopAcqusitionOnElfChange = false;
	} Settings;

	PlotHandler(std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger);
	virtual ~PlotHandler();

	std::string getLastReaderError() const;
	bool writeSeriesValue(Variable& var, double value);

	Settings getSettings() const;
	void setSettings(const Settings& newSettings);

	IDebugProbe::DebugProbeSettings getProbeSettings() const;
	void setProbeSettings(const IDebugProbe::DebugProbeSettings& settings);

	void setDebugProbe(std::shared_ptr<IDebugProbe> probe);
	double getAverageSamplingFrequency() const { return 1.0 / averageSamplingPeriod; }

   private:
	void dataHandler();
	std::vector<std::pair<uint32_t, uint8_t>> createAddressSizeVector();

   private:
	std::unique_ptr<TargetMemoryHandler> varReader;
	IDebugProbe::DebugProbeSettings probeSettings{};
	Settings settings{};
	MovingAverage samplingPeriodFilter{100};
	double averageSamplingPeriod;
};

#endif