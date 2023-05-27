#ifndef _GUI_HPP
#define _GUI_HPP

#include <map>
#include <mutex>
#include <optional>
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
	const std::map<state, std::string> viewerStateMap{{state::RUN, "RUNNING"}, {state::STOP, "STOPPED"}};
	static constexpr uint32_t maxVariableNameLength = 100;
	state viewerState = state::STOP;
	std::map<std::string, std::shared_ptr<Variable>> vars;
	std::thread threadHandle;
	PlotHandler* plotHandler;
	ConfigHandler* configHandler;
	std::string projectConfigPath;
	std::string projectElfPath;
	bool showAcqusitionSettingsWindow = false;

	std::unique_ptr<ElfReader> elfReader;

	bool& done;

	std::mutex* mtx;

	void mainThread();
	void drawMenu();
	void drawStartButton();
	void drawAddVariableButton();
	void drawUpdateAddressesFromElf();
	void drawVarTable();
	void drawAddPlotButton();
	void drawPlotsTree();
	void drawAcqusitionSettingsWindow();
	void drawPlots();
	void drawPlotCurveBar(Plot* plot, ScrollingBuffer<float>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots);
	void drawPlotTable(Plot* plot, ScrollingBuffer<float>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap);
	void showQuestionBox(const char* id, const char* question, std::function<void()> onYes, std::function<void()> onNo, std::function<void()> onCancel);
	void askShouldSaveOnExit(bool shouldOpenPopup);
	void askShouldSaveOnNew(bool shouldOpenPopup);
	void saveAs();
	void showChangeFormatPopup(const char* text, Plot& plt, const std::string& name);

	std::optional<std::string> showDeletePopup(const char* text, const std::string name);
	std::string intToHexString(uint32_t i);
};

#endif