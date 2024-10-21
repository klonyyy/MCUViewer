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
#include "GdbParser.hpp"
#include "GuiPlotEdit.hpp"
#include "GuiVariablesEdit.hpp"
#include "IDebugProbe.hpp"
#include "IFileHandler.hpp"
#include "ImguiPlugins.hpp"
#include "JlinkDebugProbe.hpp"
#include "JlinkTraceProbe.hpp"
#include "Plot.hpp"
#include "PlotGroup.hpp"
#include "PlotHandler.hpp"
#include "Popup.hpp"
#include "TracePlotHandler.hpp"
#include "imgui.h"
#include "implot.h"

class Gui
{
   public:
	Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, IFileHandler* fileHandler, TracePlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, GdbParser* parser, spdlog::logger* logger, std::string& projectPath);
	~Gui();

   private:
	static constexpr bool showDemoWindow = true;

	const std::map<PlotHandlerBase::state, std::string> viewerStateMap{{PlotHandlerBase::state::RUN, "RUNNING"}, {PlotHandlerBase::state::STOP, "STOPPED"}};
	static constexpr uint32_t maxVariableNameLength = 100;
	std::map<std::string, std::shared_ptr<Variable>> vars;
	std::thread threadHandle;
	PlotHandler* plotHandler;
	ConfigHandler* configHandler;
	std::string projectConfigPath;
	std::string projectElfPath;
	std::filesystem::file_time_type lastModifiedTime = std::filesystem::file_time_type::clock::now();
	bool showAcqusitionSettingsWindow = false;
	bool showAboutWindow = false;
	bool showPreferencesWindow = false;
	bool showImportVariablesWindow = false;
	bool performVariablesUpdate = false;

	IFileHandler* fileHandler;
	TracePlotHandler* tracePlotHandler;

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

	GdbParser* parser;

	spdlog::logger* logger;

	VariableEditWindow variableEditWindow;
	std::shared_ptr<PlotEditWindow> plotEditWindow;

	PlotGroupHandler plotGroupHandler;

   private:
	void mainThread(std::string externalPath);
	void drawMenu();
	void drawStartButton(PlotHandlerBase* activePlotHandler);
	void drawDebugProbes();
	void drawTraceProbes();
	void addNewVariable(const std::string& newName);
	void drawAddVariableButton();
	void drawUpdateAddressesFromElf();
	void drawVarTable();
	void drawAddPlotButton();
	void drawExportPlotToCSVButton(std::shared_ptr<Plot> plt);
	void drawPlotsTree();
	void drawAcqusitionSettingsWindow(ActiveViewType type);
	void acqusitionSettingsViewer();

	template <typename Settings>
	void drawLoggingSettings(PlotHandlerBase* handler, Settings& settings);
	void drawGdbSettings(PlotHandler::Settings& settings);

	void drawAboutWindow();
	void drawPreferencesWindow();
	void drawStatisticsAnalog(std::shared_ptr<Plot> plt);
	void drawStatisticsDigital(std::shared_ptr<Plot> plt);
	void acqusitionSettingsTrace();
	void renameVariable(const std::string& currentName, const std::string& newName);

	void drawPlots();
	void drawPlotCurve(std::shared_ptr<Plot> plot);
	void drawPlotBar(std::shared_ptr<Plot> plot);
	void drawPlotTable(std::shared_ptr<Plot> plot);
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
	void drawVariableEditWindow();
	void drawVariableEditSettings();
	void drawPlotEditWindow();
	void drawPlotEditSettings();

	void drawImportVariablesWindow();
	void drawImportVariablesTable(const std::map<std::string, GdbParser::VariableData>& importedVars, std::unordered_map<std::string, uint32_t>& selection, const std::string& substring);

	std::optional<std::string> showDeletePopup(const char* text, const std::string& name);

	bool openWebsite(const char* url);
};

#endif
