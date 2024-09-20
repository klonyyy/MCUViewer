#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IDebugProbe.hpp"
#include "MemoryReader.hpp"
#include "MovingAverage.hpp"
#include "Plot.hpp"
#include "PlotHandlerBase.hpp"
#include "ScrollingBuffer.hpp"
#include "spdlog/spdlog.h"

class PlotHandler : public PlotHandlerBase
{
   public:
	static constexpr uint32_t minSamplinFrequencyHz = 1;
	static constexpr uint32_t maxSamplinFrequencyHz = 1000000;
	typedef struct Settings
	{
		uint32_t sampleFrequencyHz = 100;
		uint32_t maxPoints = 10000;
		uint32_t maxViewportPoints = 5000;
		bool refreshAddressesOnElfChange = false;
		bool stopAcqusitionOnElfChange = false;
		bool shouldLog = false;
		std::string logFilePath = "";
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
	double getAverageSamplingFrequency() const
	{
		if (averageSamplingPeriod > 0.0)
			return 1.0 / averageSamplingPeriod;
		return 0.0;
	}

   private:
	void dataHandler();
	std::vector<std::pair<uint32_t, uint8_t>> createAddressSizeVector();

   private:
	static constexpr size_t maxVariablesOnSinglePlot = 100;

	std::unique_ptr<MemoryReader> varReader;
	IDebugProbe::DebugProbeSettings probeSettings{};
	Settings settings{};
	MovingAverage samplingPeriodFilter{1000};
	double averageSamplingPeriod = 0.0;
};

#endif