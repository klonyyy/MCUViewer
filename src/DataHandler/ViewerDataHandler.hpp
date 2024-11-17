#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "DataHandlerBase.hpp"
#include "IDebugProbe.hpp"
#include "MemoryReader.hpp"
#include "MovingAverage.hpp"
#include "VariableHandler.hpp"

class ViewerDataHandler : public DataHandlerBase
{
   public:
	static constexpr uint32_t minSamplinFrequencyHz = 1;
	static constexpr uint32_t maxSamplinFrequencyHz = 1000000;

	ViewerDataHandler(PlotGroupHandler* plotGroupHandler, VariableHandler* variableHandler, PlotHandler* plotHandler, TracePlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger);
	virtual ~ViewerDataHandler();

	std::string getLastReaderError() const;
	bool writeSeriesValue(Variable& var, double value);

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
	using SampleListType = std::vector<std::pair<uint32_t, uint8_t>>;

	void updateVariables(double timestamp, const std::unordered_map<uint32_t, double>& values);
	void dataHandler();
	void prepareCSVFile();
	SampleListType createSampleList();

   private:
	static constexpr size_t maxVariablesOnSinglePlot = 100;
	std::unique_ptr<MemoryReader> varReader;
	IDebugProbe::DebugProbeSettings probeSettings{};
	MovingAverage samplingPeriodFilter{1000};
	double averageSamplingPeriod = 0.0;

	std::unordered_map<std::string, double> csvEntry;

	SampleListType sampleList;
};