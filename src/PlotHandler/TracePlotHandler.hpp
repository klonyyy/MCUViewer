#ifndef _TRACEPLOTHANDLER_HPP
#define _TRACEPLOTHANDLER_HPP

#include <map>

#include "ITraceReader.hpp"
#include "Plot.hpp"
#include "PlotHandler.hpp"
#include "PlotHandlerBase.hpp"
#include "spdlog/spdlog.h"

class TracePlotHandler : public PlotHandlerBase
{
   public:
	TracePlotHandler(bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
	~TracePlotHandler();

	std::string getLastReaderError() const;

   private:
	void dataHandler();

   private:
	std::unique_ptr<ITraceReader> traceReader;
	std::map<std::string, std::shared_ptr<Variable>> traceVars;
	static constexpr uint32_t channels = 10;
	double time = 0.0;
};

#endif