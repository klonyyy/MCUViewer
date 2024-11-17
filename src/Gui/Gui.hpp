#ifndef _GUI_HPP
#define _GUI_HPP

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <unordered_set>

#include "ConfigHandler.hpp"
#include "GuiPlotEdit.hpp"
#include "GuiPlotsTree.hpp"
#include "GuiVarTable.hpp"
#include "GuiVariablesEdit.hpp"
#include "IDebugProbe.hpp"
#include "IFileHandler.hpp"
#include "ImguiPlugins.hpp"
#include "JlinkDebugProbe.hpp"
#include "JlinkTraceProbe.hpp"
#include "Plot.hpp"
#include "PlotGroupHandler.hpp"
#include "Popup.hpp"
#include "TraceDataHandler.hpp"
#include "VariableHandler.hpp"
#include "ViewerDataHandler.hpp"
#include "imgui.h"
#include "implot.h"

class Gui
{
   public:
	Gui(PlotHandler* plotHandler, VariableHandler* variableHandler, ConfigHandler* configHandler, PlotGroupHandler* plotGroupHandler, IFileHandler* fileHandler, PlotHandler* tracePlotHandler, ViewerDataHandler* viewerDataHandler, TraceDataHandler* traceDataHandler, std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger, std::string& projectPath);
	~Gui();

   private:
	static constexpr bool showDemoWindow = true;

	const std::map<DataHandlerBase::state, std::string> viewerStateMap{{DataHandlerBase::state::RUN, "RUNNING"}, {DataHandlerBase::state::STOP, "STOPPED"}};

	std::thread threadHandle;
	PlotHandler* plotHandler;
	VariableHandler* variableHandler;
	ConfigHandler* configHandler;
	PlotGroupHandler* plotGroupHandler;

	std::string projectConfigPath;
	std::string projectElfPath;
	bool showAcqusitionSettingsWindow = false;
	bool showAboutWindow = false;
	bool showPreferencesWindow = false;
	bool showSelectVariablesWindow = false;

	IFileHandler* fileHandler;
	PlotHandler* tracePlotHandler;
	ViewerDataHandler* viewerDataHandler;
	TraceDataHandler* traceDataHandler;

	std::shared_ptr<IDebugProbe> stlinkProbe;
	std::shared_ptr<IDebugProbe> jlinkProbe;
	std::shared_ptr<IDebugProbe> debugProbeDevice;
	std::vector<std::string> devicesList{};
	const std::string noDevices = "No debug probes found!";

	std::shared_ptr<ITraceProbe> stlinkTraceProbe;
	std::shared_ptr<ITraceProbe> jlinkTraceProbe;
	std::shared_ptr<ITraceProbe> traceProbeDevice;

	std::atomic<bool>& done;

	Popup popup;
	Popup acqusitionErrorPopup;

	enum class ActiveViewType : uint8_t
	{
		VarViewer = 0,
		TraceViewer = 1,
	};

	ActiveViewType activeView = ActiveViewType::VarViewer;

	std::mutex* mtx;

	spdlog::logger* logger;

	std::shared_ptr<PlotEditWindow> plotEditWindow;
	std::shared_ptr<VariableTableWindow> variableTable;
	std::shared_ptr<PlotsTree> plotsTree;

   private:
	void mainThread(std::string externalPath);
	void drawMenu();
	void drawStartButton(DataHandlerBase* activeDataHandler);
	void drawDebugProbes();
	void drawTraceProbes();
	void drawUpdateAddressesFromElf();
	void drawAcqusitionSettingsWindow(ActiveViewType type);
	void acqusitionSettingsViewer();

	template <typename Settings>
	void drawLoggingSettings(PlotHandler* handler, Settings& settings);
	void drawGdbSettings(ViewerDataHandler::Settings& settings);

	void drawAboutWindow();
	void drawPreferencesWindow();
	void acqusitionSettingsTrace();

	void drawPlots();
	void drawPlotCurve(std::shared_ptr<Plot> plot);
	void drawPlotBar(std::shared_ptr<Plot> plot);
	void drawPlotTable(std::shared_ptr<Plot> plot);
	void drawPlotXY(std::shared_ptr<Plot> plot);
	void handleMarkers(uint32_t id, Plot::Marker& marker, ImPlotRect plotLimits, std::function<void()> activeCallback);
	void handleDragRect(uint32_t id, Plot::DragRect& dragRect, ImPlotRect plotLimits);
	void dragAndDropPlot(std::shared_ptr<Plot> plot);

	void showQuestionBox(const char* id, const char* question, std::function<void()> onYes, std::function<void()> onNo, std::function<void()> onCancel);
	void askShouldSaveOnExit(bool shouldOpenPopup);
	void askShouldSaveOnNew(bool shouldOpenPopup);
	bool saveProject();
	bool saveProjectAs();
	void showChangeFormatPopup(const char* text, Plot& plt, const std::string& name);
	bool openElfFile();
	bool openLogDirectory(std::string& logDirectory);
	std::string convertProjectPathToAbsolute(const std::string& projectRelativePath);
	void checkShortcuts();
	bool checkElfFileChanged();
	bool openProject(std::string externalPath = "");
	void drawSettingsSwo();
	void drawIndicatorsSwo();
	void drawPlotsSwo();
	void drawPlotCurveSwo(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, bool first);
	void drawPlotsTreeSwo();

	bool openWebsite(const char* url);
};

#endif
