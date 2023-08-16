#ifndef _TRACEPLOTHANDLER_HPP
#define _TRACEPLOTHANDLER_HPP

#include <map>

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
		uint32_t coreFrequency;
		uint32_t traceFrequency;
	} TraceSettings;

	TraceSettings traceSettings{};

	TracePlotHandler(std::atomic<bool>& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
	~TracePlotHandler();

	TraceSettings getTraceSettings() const;
	void setTraceSettings(const TraceSettings& settings);

	std::map<const char*, uint32_t> getTraceIndicators() const;

	std::string getLastReaderError() const;

   private:
	void dataHandler();

   private:
	std::shared_ptr<StlinkTraceDevice> traceDevice;
	std::unique_ptr<TraceReader> traceReader;
	std::map<std::string, std::shared_ptr<Variable>> traceVars;
	static constexpr uint32_t channels = 10;
	double time = 0.0;
};

#endif