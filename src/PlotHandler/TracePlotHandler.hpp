#ifndef _TRACEPLOTHANDLER_HPP
#define _TRACEPLOTHANDLER_HPP

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Plot.hpp"
#include "PlotHandlerBase.hpp"

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
		bool shouldReset = false;
		uint32_t timeout = 2;
		bool shouldLog = false;
		std::string logFilePath = "";
	} Settings;

	void initPlots();

	Settings getSettings() const;
	void setSettings(const Settings& settings);

	double getDoubleValue(const Plot& plot, uint32_t value);
	std::map<std::string, std::shared_ptr<Variable>> traceVars;

   private:
	Settings settings{};
	static constexpr uint32_t channels = 10;
};

#endif