#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IDebugProbe.hpp"
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
	} Settings;

	typedef struct
	{
		uint32_t debugProbe = 0;
		std::string serialNumber = "";
		std::string device = "";
		IDebugProbe::Mode mode = IDebugProbe::Mode::NORMAL;
		uint32_t speedkHz = 100;

	} DebugProbeSettings;

	PlotHandler(std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger);
	virtual ~PlotHandler();

	std::string getLastReaderError() const;
	bool writeSeriesValue(Variable& var, double value);

	Settings getSettings() const;
	void setSettings(const Settings& newSettings);

	DebugProbeSettings getProbeSettings() const;
	void setProbeSettings(const DebugProbeSettings& settings);

	void setDebugProbe(std::shared_ptr<IDebugProbe> probe);
	double getAverageSamplingPeriod() const { return averageSamplingFrequency; }

   private:
	void dataHandler();
	std::vector<std::pair<uint32_t, uint8_t>> createAddressSizeVector();

   private:
	std::unique_ptr<TargetMemoryHandler> varReader;
	std::chrono::time_point<std::chrono::steady_clock> start;
	DebugProbeSettings probeSettings{};
	Settings settings{};
	double averageSamplingFrequency;
};

#endif