#pragma once

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "DataHandlerBase.hpp"
#include "Plot.hpp"
#include "StlinkTraceProbe.hpp"
#include "TraceReader.hpp"
#include "spdlog/spdlog.h"

class TraceDataHandler : public DataHandlerBase
{
   public:
	struct Settings
	{
		uint32_t coreFrequency = 160000;
		uint32_t tracePrescaler = 10;
		uint32_t maxPoints = 10000;
		uint32_t maxViewportPointsPercent = 10;
		int32_t triggerChannel = -1;
		double triggerLevel = 0.9;
		bool shouldReset = false;
		uint32_t timeout = 2;
		bool shouldLog = false;
		std::string logFilePath = "";
	};

	TraceDataHandler(PlotGroupHandler* plotGroupHandler, VariableHandler* variableHandler, PlotHandler* plotHandler, PlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger);
	~TraceDataHandler();

	TraceReader::TraceIndicators getTraceIndicators() const;
	std::vector<double> getErrorTimestamps();
	std::vector<double> getDelayed3Timestamps();
	std::string getLastReaderError() const;

	void setTriggerChannel(int32_t triggerChannel);
	int32_t getTriggerChannel() const;

	void setDebugProbe(std::shared_ptr<ITraceProbe> probe);
	ITraceProbe::TraceProbeSettings getProbeSettings() const;
	void setProbeSettings(const ITraceProbe::TraceProbeSettings& settings);

	Settings getSettings() const;
	void setSettings(const Settings& settings);

	/* TODO refactor so these are not here: (ideally using traceVeriableHandler)*/
	double getDoubleValue(const Plot& plot, uint32_t value);
	void initPlots();
	std::map<std::string, std::shared_ptr<Variable>> traceVars;

   private:
	void dataHandler();
	void prepareCSVFile();

   private:
	class MarkerTimestamps
	{
	   public:
		void reset()
		{
			timestamps.clear();
			previousErrors = 0;
		}

		void handle(double time, double oldestTimestamp, uint32_t totalErrors)
		{
			if (previousErrors != totalErrors)
				timestamps.push_back(time);

			while (timestamps.size() && timestamps.front() < oldestTimestamp)
				timestamps.pop_front();
			previousErrors = totalErrors;
		}

		size_t size() const
		{
			return timestamps.size();
		}

		auto getVector()
		{
			return std::vector<double>(timestamps.begin(), timestamps.end());
		}

	   private:
		std::deque<double> timestamps;
		uint32_t previousErrors;
	};

	Settings settings{};
	std::unique_ptr<TraceReader> traceReader;

	MarkerTimestamps errorFrames{};
	MarkerTimestamps delayed3Frames{};

	std::string lastErrorMsg{};

	ITraceProbe::TraceProbeSettings probeSettings;

	bool traceTriggered = false;
	static constexpr uint32_t channels = 10;
	static constexpr size_t maxAllowedViewportErrors = 100;

	std::unordered_map<std::string, double> csvEntry;
};
