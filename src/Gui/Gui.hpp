#ifndef _GUI_HPP
#define _GUI_HPP

#include <map>
#include <mutex>
#include <optional>
#include <thread>

#include "ConfigHandler.hpp"
#include "ElfReader.hpp"
#include "IFileHandler.hpp"
#include "ImguiPlugins.hpp"
#include "Plot.hpp"
#include "PlotHandler.hpp"
#include "imgui.h"
#include "implot.h"

class Gui
{
   public:
	Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, IFileHandler* fileHandler, bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
	~Gui();

   private:
	const std::map<PlotHandler::state, std::string> viewerStateMap{{PlotHandler::state::RUN, "RUNNING"}, {PlotHandler::state::STOP, "STOPPED"}};
	static constexpr uint32_t maxVariableNameLength = 100;
	std::map<std::string, std::shared_ptr<Variable>> vars;
	std::thread threadHandle;
	PlotHandler* plotHandler;
	ConfigHandler* configHandler;
	std::string projectConfigPath;
	std::string projectElfPath;
	bool showAcqusitionSettingsWindow = false;

	std::unique_ptr<ElfReader> elfReader;
	IFileHandler* fileHandler;

	bool& done;

	std::mutex* mtx;

	ConfigHandler::Settings settings;

	void mainThread();
	void drawMenu();
	void drawStartButton();
	void drawAddVariableButton();
	void drawUpdateAddressesFromElf();
	void drawVarTable();
	void drawAddPlotButton();
	void drawExportPlotToCSVButton(std::shared_ptr<Plot> plt);
	void drawPlotsTree();
	void drawAcqusitionSettingsWindow();
	void drawPlots();
	void drawPlotCurve(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots);
	void drawPlotBar(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots);
	void drawPlotTable(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap);
	void showQuestionBox(const char* id, const char* question, std::function<void()> onYes, std::function<void()> onNo, std::function<void()> onCancel);
	void askShouldSaveOnExit(bool shouldOpenPopup);
	void askShouldSaveOnNew(bool shouldOpenPopup);
	bool saveProject();
	bool saveProjectAs();
	void showChangeFormatPopup(const char* text, Plot& plt, const std::string& name);
	bool openProject();
	bool openElfFile();
	void checkShortcuts();

	std::optional<std::string> showDeletePopup(const char* text, const std::string name);
	std::string intToHexString(uint32_t i);

	std::shared_ptr<spdlog::logger> logger;
};

#endif