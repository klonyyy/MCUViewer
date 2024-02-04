#ifndef _GUI_HPP
#define _GUI_HPP

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>

#include "ConfigHandler.hpp"
#include "ElfReader.hpp"
#include "GdbParser.hpp"
#include "IFileHandler.hpp"
#include "ImguiPlugins.hpp"
#include "Plot.hpp"
#include "PlotHandler.hpp"
#include "Popup.hpp"
#include "TracePlotHandler.hpp"
#include "imgui.h"
#include "implot.h"

class Gui
{
   public:
	Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, IFileHandler* fileHandler, TracePlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, GdbParser* parser, spdlog::logger* logger);
	~Gui();

   private:
	const std::map<PlotHandlerBase::state, std::string> viewerStateMap{{PlotHandlerBase::state::RUN, "RUNNING"}, {PlotHandlerBase::state::STOP, "STOPPED"}};
	static constexpr uint32_t maxVariableNameLength = 100;
	std::map<std::string, std::shared_ptr<Variable>> vars;
	std::thread threadHandle;
	PlotHandler* plotHandler;
	ConfigHandler* configHandler;
	std::string projectConfigPath;
	std::string projectElfPath;
	bool showAcqusitionSettingsWindow = false;
	bool showAboutWindow = false;
	bool showPreferencesWindow = false;
	bool showImportVariablesWindow = false;

	std::unique_ptr<ElfReader> elfReader;
	IFileHandler* fileHandler;

	TracePlotHandler* tracePlotHandler;

	std::atomic<bool>& done;

	Popup popup;

	enum class AcqusitionWindowType : uint8_t
	{
		VARIABLE = 0,
		TRACE = 1,
	};

	std::mutex* mtx;

	GdbParser* parser;

	void mainThread();
	void drawMenu();
	void drawStartButton();
	void addNewVariable(const std::string& newName);
	void drawAddVariableButton();
	void drawUpdateAddressesFromElf();
	void drawVarTable();
	void drawAddPlotButton();
	void drawExportPlotToCSVButton(std::shared_ptr<Plot> plt);
	void drawPlotsTree();
	void drawAcqusitionSettingsWindow(AcqusitionWindowType type);
	void acqusitionSettingsViewer();
	void drawAboutWindow();
	void drawPreferencesWindow();
	void drawStatisticsAnalog(std::shared_ptr<Plot> plt);
	void drawStatisticsDigital(std::shared_ptr<Plot> plt);
	void acqusitionSettingsTrace();

	void drawPlots();
	void drawPlotCurve(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots);
	void drawPlotBar(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots);
	void drawPlotTable(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap);
	void handleMarkers(uint32_t id, Plot::Marker& marker, ImPlotRect plotLimits, std::function<void()> activeCallback);
	void handleDragRect(uint32_t id, Plot::DragRect& dragRect, ImPlotRect plotLimits);

	void showQuestionBox(const char* id, const char* question, std::function<void()> onYes, std::function<void()> onNo, std::function<void()> onCancel);
	void askShouldSaveOnExit(bool shouldOpenPopup);
	void askShouldSaveOnNew(bool shouldOpenPopup);
	bool saveProject();
	bool saveProjectAs();
	void showChangeFormatPopup(const char* text, Plot& plt, const std::string& name);
	bool openProject();
	bool openElfFile();
	void checkShortcuts();

	void drawStartButtonSwo();
	void drawSettingsSwo();
	void drawIndicatorsSwo();
	void drawPlotsSwo();
	void drawPlotCurveSwo(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, bool first);
	void drawPlotsTreeSwo();

	void drawImportVariablesWindow();
	void drawImportVariablesTable(const std::vector<GdbParser::VariableData>& importedVars, std::unordered_map<std::string, uint32_t>& selection, const std::string& substring);

	template <typename T>
	void drawInputText(const char* id, T variable, std::function<void(std::string)> valueChanged)
	{
		std::string str = std::to_string(variable);
		if (ImGui::InputText(id, &str, ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL) || ImGui::IsMouseClicked(0))
			if (valueChanged)
				valueChanged(str);
	}
	template <typename T>
	void drawDescriptionWithNumber(const char* description, T number)
	{
		ImGui::Text(description);
		ImGui::SameLine();
		ImGui::Text("%s", (std::to_string(number)).c_str());
	}

	std::optional<std::string> showDeletePopup(const char* text, const std::string& name);
	std::string intToHexString(uint32_t i);
	void drawCenteredText(std::string&& text);

	bool openWebsite(const char* url);

	spdlog::logger* logger;
};

#endif