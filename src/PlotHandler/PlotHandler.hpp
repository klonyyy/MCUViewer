#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include <map>
#include <memory>

#include <string>


#include "Plot.hpp"
#include "PlotHandlerBase.hpp"

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
		bool shouldLog = false;
		std::string logFilePath = "";
		std::string gdbCommand = "gdb";
	} Settings;

	Settings getSettings() const;
	void setSettings(const Settings& newSettings);

   private:
	Settings settings{};
};

#endif