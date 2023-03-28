#ifndef _GUI_HPP
#define _GUI_HPP

#include <mutex>
#include <thread>
#include <map>

#include "ConfigHandler.hpp"
#include "ElfReader.hpp"
#include "Plot.hpp"
#include "PlotHandler.hpp"
#include "imgui.h"

class Gui
{
   public:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};

	Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, bool& done);
	~Gui();

   private:
	state viewerState = state::STOP;
	const std::map<state, std::string> viewerStateMap{{state::RUN, "RUNNING"}, {state::STOP, "STOPPED"}};
	std::map<std::string, std::shared_ptr<Variable>> vars;
	std::thread threadHandle;
	PlotHandler* plotHandler;
	ConfigHandler* configHandler;
	std::mutex mtx;
	std::string projectConfigFile;
	std::string projectElfFile;
	bool showAcqusitionSettingsWindow = false;

	static constexpr uint32_t maxVariableNameLength = 100;

	std::unique_ptr<ElfReader> elfReader;

	bool& done;
	void mainThread();
	void drawMenu();
	void drawStartButton();
	void drawAddVariableButton();
	void drawUpdateAddressesFromElf();
	void drawVarTable();
	void drawPlotsTree();
	void drawAcqusitionSettingsWindow();
	void drawPlot(Plot* plot, ScrollingBuffer<float>& time, std::map<uint32_t, std::shared_ptr<Plot::Series>>& seriesMap);
	std::string intToHexString(uint32_t i);
};

#endif