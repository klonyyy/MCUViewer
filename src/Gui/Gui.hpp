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
#include "TracePlotHandler.hpp"
#include "imgui.h"
#include "implot.h"

class Gui
{
   public:
	Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, IFileHandler* fileHandler, TracePlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
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

	std::unique_ptr<ElfReader> elfReader;
	IFileHandler* fileHandler;

	TracePlotHandler* tracePlotHandler;

	std::atomic<bool>& done;

	enum class AcqusitionWindowType : uint8_t
	{
		VARIABLE = 0,
		TRACE = 1,
	};

	std::mutex* mtx;

	void mainThread();
	void drawMenu();
	void drawStartButton();
	void drawAddVariableButton();
	void drawUpdateAddressesFromElf();
	void drawVarTable();
	void drawAddPlotButton();
	void drawExportPlotToCSVButton(std::shared_ptr<Plot> plt);
	void drawPlotsTree();
	void drawAcqusitionSettingsWindow(AcqusitionWindowType type);
	void acqusitionSettingsViewer();
	void drawAboutWindow();
	void acqusitionSettingsTrace();
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

	void drawStartButtonSwo();
	void drawSettingsSwo();
	void drawIndicatorsSwo();
	void drawPlotsSwo();
	void drawPlotCurveSwo(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, bool first);
	void drawPlotsTreeSwo();

	template <typename T>
	void drawInputText(const char* id, T variable, std::function<void(std::string)> valueChanged)
	{
		std::string str = std::to_string(variable);

		ImGui::InputText(id, &str, 0, NULL, NULL);

		if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && str != std::to_string(variable))
		{
			if (valueChanged)
				valueChanged(str);
		}
	}

	std::optional<std::string> showDeletePopup(const char* text, const std::string name);
	void showSavedPopup(bool show);
	std::string intToHexString(uint32_t i);
	void drawCenteredText(std::string&& text);

	bool openWebsite(const char* url);

	std::shared_ptr<spdlog::logger> logger;
};

#endif