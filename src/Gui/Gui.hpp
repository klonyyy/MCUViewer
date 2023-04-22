#ifndef _GUI_HPP
#define _GUI_HPP

#include <map>
#include <mutex>
#include <thread>

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

	Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, bool& done, std::mutex* mtx);
	~Gui();

   private:
	state viewerState = state::STOP;
	const std::map<state, std::string> viewerStateMap{{state::RUN, "RUNNING"}, {state::STOP, "STOPPED"}};
	std::map<std::string, std::shared_ptr<Variable>> vars;
	std::thread threadHandle;
	PlotHandler* plotHandler;
	ConfigHandler* configHandler;
	std::string projectConfigFile;
	std::string projectElfFile;
	bool showAcqusitionSettingsWindow = false;

	static constexpr uint32_t maxVariableNameLength = 100;

	std::unique_ptr<ElfReader> elfReader;

	bool& done;

	std::mutex* mtx;

	void mainThread();
	void drawMenu();
	void drawStartButton();
	void drawAddVariableButton();
	void drawUpdateAddressesFromElf();
	void drawVarTable();
	void drawPlotsTree();
	void drawAcqusitionSettingsWindow();
	void drawPlotCurveBar(Plot* plot, ScrollingBuffer<float>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots);
	void drawPlotTable(Plot* plot, ScrollingBuffer<float>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap);
	bool showSaveOnClosePrompt();

	std::string showDeletePopup(const char* text, const std::string name);
	std::string intToHexString(uint32_t i);
};

#endif