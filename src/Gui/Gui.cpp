#include "Gui.hpp"

#include <unistd.h>

#include <future>
#include <random>
#include <sstream>
#include <string>
#include <utility>

#include "PlotHandlerBase.hpp"
#include "Statistics.hpp"
#include "glfw3.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifdef _WIN32
#include <windows.h>
#endif

Gui::Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, IFileHandler* fileHandler, TracePlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, GdbParser* parser, spdlog::logger* logger) : plotHandler(plotHandler), configHandler(configHandler), fileHandler(fileHandler), tracePlotHandler(tracePlotHandler), done(done), mtx(mtx), parser(parser), logger(logger)
{
	threadHandle = std::thread(&Gui::mainThread, this);
}

Gui::~Gui()
{
	if (threadHandle.joinable())
		threadHandle.join();
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void Gui::mainThread()
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return;

	GLFWwindow* window = glfwCreateWindow(1500, 1000, (std::string("STMViewer | ") + projectConfigPath).c_str(), NULL, NULL);
	if (window == NULL)
		return;
	glfwMakeContextCurrent(window);
	glfwMaximizeWindow(window);
	glfwSwapInterval(2);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();
	ImPlot::StyleColorsDark();

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

	fileHandler->init();

	bool show_demo_window = false;

	while (!done)
	{
		glfwSetWindowTitle(window, (std::string("STMViewer - ") + projectConfigPath).c_str());
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		if (show_demo_window)
			ImPlot::ShowDemoWindow();

		if (glfwWindowShouldClose(window))
			askShouldSaveOnExit(glfwWindowShouldClose(window));
		glfwSetWindowShouldClose(window, done);
		checkShortcuts();

		drawMenu();
		drawAboutWindow();
		drawPreferencesWindow();

		if (ImGui::Begin("Trace Viewer"))
		{
			drawAcqusitionSettingsWindow(AcqusitionWindowType::TRACE);
			ImGui::SetNextWindowClass(&window_class);
			if (ImGui::Begin("Trace Plots"))
				drawPlotsSwo();
			ImGui::End();
			drawStartButtonSwo();
			drawSettingsSwo();
			drawIndicatorsSwo();
			drawPlotsTreeSwo();
		}
		ImGui::End();

		if (ImGui::Begin("Var Viewer"))
		{
			drawAcqusitionSettingsWindow(AcqusitionWindowType::VARIABLE);
			drawStartButton();
			drawVarTable();
			drawPlotsTree();
			drawImportVariablesWindow();
			ImGui::SetNextWindowClass(&window_class);
			if (ImGui::Begin("Plots"))
				drawPlots();
			ImGui::End();
		}
		ImGui::End();

		popup.handle();

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);
	}

	logger->info("Exiting GUI main thread");

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	fileHandler->deinit();
}

void Gui::drawMenu()
{
	bool shouldSaveOnClose = false;
	bool shouldSaveOnNew = false;
	ImGui::BeginMainMenuBar();

	bool active = !(plotHandler->getViewerState() == PlotHandlerBase::state::RUN || tracePlotHandler->getViewerState() == PlotHandlerBase::state::RUN);

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New"))
			shouldSaveOnNew = true;

		if (ImGui::MenuItem("Open", "Ctrl+O"))
			openProject();

		if (ImGui::MenuItem("Save", "Ctrl+S", false, (!projectConfigPath.empty())))
			saveProject();

		if (ImGui::MenuItem("Save As.."))
			saveProjectAs();

		if (ImGui::MenuItem("Quit"))
			shouldSaveOnClose = true;

		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Options"))
	{
		ImGui::MenuItem("Acquisition settings...", NULL, &showAcqusitionSettingsWindow, active);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Preferences", NULL, &showPreferencesWindow, active);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Help"))
	{
		ImGui::MenuItem("About", NULL, &showAboutWindow, active);
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
	askShouldSaveOnExit(shouldSaveOnClose);
	askShouldSaveOnNew(shouldSaveOnNew);
}

void Gui::drawStartButton()
{
	PlotHandlerBase::state state = plotHandler->getViewerState();

	if (state == PlotHandlerBase::state::RUN)
	{
		ImVec4 color = (ImVec4)ImColor::HSV(0.365f, 0.94f, 0.37f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}
	else if (state == PlotHandlerBase::state::STOP)
	{
		ImVec4 color = ImColor::HSV(0.116f, 0.97f, 0.72f);

		if (plotHandler->getLastReaderError() != "")
			color = ImColor::HSV(0.0f, 0.95f, 0.70f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}

	if (ImGui::Button((viewerStateMap.at(state) + " " + plotHandler->getLastReaderError()).c_str(), ImVec2(-1, 50)))
	{
		if (state == PlotHandlerBase::state::STOP)
		{
			plotHandler->eraseAllPlotData();
			plotHandler->setViewerState(PlotHandlerBase::state::RUN);
		}
		else
			plotHandler->setViewerState(PlotHandlerBase::state::STOP);
	}

	ImGui::PopStyleColor(3);
}

void Gui::addNewVariable(const std::string& newName)
{
	std::shared_ptr<Variable> newVar = std::make_shared<Variable>(newName);
	std::random_device rd{};
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<uint32_t> dist{0, UINT32_MAX};
	uint32_t randomColor = dist(gen);
	newVar->setColor(randomColor);
	vars.emplace(newName, newVar);
}

void Gui::drawAddVariableButton()
{
	if (ImGui::Button("Add variable", ImVec2(-1, 25)))
	{
		uint32_t num = 0;
		while (vars.find(std::string("-new") + std::to_string(num)) != vars.end())
			num++;

		addNewVariable(std::string("-new") + std::to_string(num));
	}

	ImGui::BeginDisabled(projectElfPath.empty());

	if (ImGui::Button("Import variables from *.elf", ImVec2(-1, 25)))
		showImportVariablesWindow = true;

	ImGui::EndDisabled();
}

void Gui::drawUpdateAddressesFromElf()
{
	static std::future<bool> refreshThread{};

	char buttonText[30]{};

	if (refreshThread.valid() && refreshThread.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		snprintf(buttonText, 30, "Update variable addresses %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
	else
	{
		snprintf(buttonText, 30, "Update variable addresses");
		if (refreshThread.valid() && !refreshThread.get())
			popup.show("Error!", "Update error. Please check the *.elf file path!", 2.0f);
	}

	ImGui::BeginDisabled(projectElfPath.empty());

	if (ImGui::Button(buttonText, ImVec2(-1, 25)))
		refreshThread = std::async(std::launch::async, &GdbParser::updateVariableMap2, parser, projectElfPath, std::ref(vars));

	ImGui::EndDisabled();
}

void Gui::drawVarTable()
{
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Variables").x) * 0.5f);
	ImGui::Text("Variables");
	ImGui::Separator();

	drawAddVariableButton();
	drawUpdateAddressesFromElf();

	if (ImGui::BeginTable("table_scrolly", 3, flags, ImVec2(0.0f, 300)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name", 0);
		ImGui::TableSetupColumn("Address", 0);
		ImGui::TableSetupColumn("Type", 0);
		ImGui::TableHeadersRow();

		std::optional<std::string> varNameToDelete;

		for (auto& [keyName, var] : vars)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(keyName.c_str());
			ImGui::ColorEdit4("##", &var->getColor().r, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::PopID();
			char variable[maxVariableNameLength] = {0};
			std::memcpy(variable, var->getName().data(), (var->getName().length()));
			ImGui::SelectableInput(var->getName().c_str(), false, ImGuiSelectableFlags_None, variable, maxVariableNameLength);
			if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter) || ImGui::IsMouseClicked(0))
			{
				auto varr = vars.extract(var->getName());
				varr.key() = std::string(variable);
				var->setName(variable);
				vars.insert(std::move(varr));
			}

			if (!varNameToDelete.has_value())
				varNameToDelete = showDeletePopup("Delete", keyName);

			if (var->getIsFound() == true && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload("MY_DND", &var->getName(), sizeof(var->getName()));
				ImPlot::ItemIcon(var->getColorU32());
				ImGui::SameLine();
				ImGui::TextUnformatted(var->getName().c_str());
				ImGui::EndDragDropSource();
			}
			ImGui::TableSetColumnIndex(1);
			if (var->getIsFound() == true)
				ImGui::Text("%s", ("0x" + std::string(intToHexString(var->getAddress()))).c_str());
			else
				ImGui::Text("NOT FOUND!");
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%s", var->getTypeStr().c_str());
		}
		if (varNameToDelete.has_value())
		{
			for (std::shared_ptr<Plot> plt : *plotHandler)
				plt->removeSeries(varNameToDelete.value_or(""));
			vars.erase(varNameToDelete.value_or(""));
		}
		ImGui::EndTable();
	}
}

void Gui::drawAddPlotButton()
{
	if (ImGui::Button("Add plot", ImVec2(-1, 25)))
	{
		uint32_t num = 0;
		while (plotHandler->checkIfPlotExists(std::string("new plot") + std::to_string(num)))
			num++;

		std::string newName = std::string("new plot") + std::to_string(num);
		plotHandler->addPlot(newName);
	}
}

void Gui::drawExportPlotToCSVButton(std::shared_ptr<Plot> plt)
{
	if (ImGui::Button("Export plot to *.csv", ImVec2(-1, 25)))
	{
		std::string path = fileHandler->saveFile(std::pair<std::string, std::string>("CSV", "csv"));
		std::ofstream csvFile(path);

		if (!csvFile)
		{
			logger->info("Error opening the file: {}", path);
			return;
		}

		uint32_t dataSize = plt->getTimeSeries().getSize();

		csvFile << "time [s],";

		for (auto& [name, ser] : plt->getSeriesMap())
			csvFile << name << ",";

		csvFile << std::endl;

		for (size_t i = 0; i < dataSize; ++i)
		{
			uint32_t offset = plt->getTimeSeries().getOffset();
			uint32_t index = (offset + i < dataSize) ? offset + i : i - (dataSize - offset);
			csvFile << plt->getTimeSeries().getFirstElementCopy()[index] << ",";

			for (auto& [name, ser] : plt->getSeriesMap())
				csvFile << ser->buffer->getFirstElementCopy()[index] << ",";

			csvFile << std::endl;
		}

		csvFile.close();
	}
}

void Gui::drawPlotsTree()
{
	const uint32_t windowHeight = 320;
	const char* plotTypes[3] = {"curve", "bar", "table"};
	static std::string selected = "";
	std::optional<std::string> plotNameToDelete = {};

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Plots").x) * 0.5f);
	ImGui::Text("Plots");
	ImGui::Separator();

	drawAddPlotButton();

	if (plotHandler->getPlotsCount() == 0)
	{
		plotHandler->addPlot("new plot0");
		selected = std::string("new plot0");
	}

	if (!plotHandler->checkIfPlotExists(std::move(selected)))
		selected = plotHandler->begin().operator*()->getName();

	ImGui::BeginChild("Plot Tree", ImVec2(-1, windowHeight));
	ImGui::BeginChild("left pane", ImVec2(150, -1), true);

	for (std::shared_ptr<Plot> plt : *plotHandler)
	{
		std::string name = plt->getName();
		ImGui::Checkbox(std::string("##" + name).c_str(), &plt->getVisibilityVar());
		ImGui::SameLine();
		if (ImGui::Selectable(name.c_str(), selected == name))
			selected = name;

		if (!plotNameToDelete.has_value())
			plotNameToDelete = showDeletePopup("Delete plot", name);

		if (plt->isHovered() && ImGui::IsMouseClicked(0))
			selected = plt->getName();
	}

	ImGui::EndChild();
	ImGui::SameLine();

	std::shared_ptr<Plot> plt = plotHandler->getPlot(selected);
	std::string newName = plt->getName();
	int32_t typeCombo = (int32_t)plt->getType();
	ImGui::BeginGroup();
	ImGui::Text("name       ");
	ImGui::SameLine();
	ImGui::PushID(plt->getName().c_str());
	ImGui::InputText("##input", &newName, 0, NULL, NULL);
	ImGui::Text("type       ");
	ImGui::SameLine();
	ImGui::Combo("##combo", &typeCombo, plotTypes, IM_ARRAYSIZE(plotTypes));
	/* Staticstics */
	if (typeCombo == 0)
	{
		bool mx0 = plt->markerX0.getState();
		bool mx1 = plt->markerX1.getState();
		ImGui::Text("x0 marker  ");
		ImGui::SameLine();
		ImGui::Checkbox("##mx0", &mx0);
		plt->markerX0.setState(mx0);
		ImGui::Text("x1 marker  ");
		ImGui::SameLine();
		ImGui::Checkbox("##mx1", &mx1);
		plt->markerX1.setState(mx1);
		drawStatisticsAnalog(plt);
	}
	ImGui::PopID();

	/* Var list within plot*/
	ImGui::PushID("list");
	if (ImGui::BeginListBox("##", ImVec2(-1, 175)))
	{
		std::optional<std::string> seriesNameToDelete = {};
		for (auto& [name, ser] : plt->getSeriesMap())
		{
			ImGui::PushID(name.c_str());
			ImGui::Checkbox("", &ser->visible);
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID(name.c_str());
			ImGui::ColorEdit4("##", &ser->var->getColor().r, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::Selectable(name.c_str());
			if (!seriesNameToDelete.has_value())
				seriesNameToDelete = showDeletePopup("Delete var", name);
			ImGui::PopID();
		}
		plt->removeSeries(seriesNameToDelete.value_or(""));

		ImGui::EndListBox();
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
			plt->addSeries(*vars[*(std::string*)payload->Data]);
		ImGui::EndDragDropTarget();
	}
	drawExportPlotToCSVButton(plt);
	ImGui::PopID();
	ImGui::EndGroup();
	ImGui::EndChild();

	if (typeCombo != (int32_t)plt->getType())
		plt->setType(static_cast<Plot::Type>(typeCombo));

	if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter) || ImGui::IsMouseClicked(0)) && newName != plt->getName())
	{
		if (!plotHandler->checkIfPlotExists(std::move(newName)))
		{
			plotHandler->renamePlot(plt->getName(), newName);
			selected = newName;
		}
		else
			popup.show("Error", "Plot already exists!", 1.5f);
	}
	if (plotNameToDelete.has_value())
		plotHandler->removePlot(plotNameToDelete.value_or(""));
}

void Gui::drawAcqusitionSettingsWindow(AcqusitionWindowType type)
{
	if (showAcqusitionSettingsWindow)
		ImGui::OpenPopup("Acqusition Settings");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500, 300));
	if (ImGui::BeginPopupModal("Acqusition Settings", &showAcqusitionSettingsWindow, 0))
	{
		if (type == AcqusitionWindowType::VARIABLE)
			acqusitionSettingsViewer();
		else if (type == AcqusitionWindowType::TRACE)
			acqusitionSettingsTrace();

		acqusitionErrorPopup.handle();

		const float buttonHeight = 25.0f;
		ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));

		if (ImGui::Button("Done", ImVec2(-1, buttonHeight)))
		{
			showAcqusitionSettingsWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Gui::acqusitionSettingsViewer()
{
	ImGui::Text("Project's *.elf file:");
	ImGui::InputText("##", &projectElfPath, 0, NULL, NULL);
	ImGui::SameLine();
	if (ImGui::Button("...", ImVec2(35, 19)))
		openElfFile();

	PlotHandler::Settings settings = plotHandler->getSettings();

	ImGui::Text("Sample period [ms]:");
	ImGui::SameLine();
	ImGui::HelpMarker("Minimum time between two respective sampling points. Set to zero for maximum frequency.");
	static int one = 1;
	ImGui::InputScalar("##sample", ImGuiDataType_U32, &settings.samplePeriod, &one, NULL, "%u");
	settings.samplePeriod = std::clamp(settings.samplePeriod, static_cast<uint32_t>(0), static_cast<uint32_t>(1000));

	const uint32_t minPoints = 100;
	const uint32_t maxPoints = 20000;
	ImGui::Text("Max points [100 - 20000]:");
	ImGui::SameLine();
	ImGui::HelpMarker("Max points used for a single series after which the oldest points will be overwritten.");
	ImGui::InputScalar("##maxPoints", ImGuiDataType_U32, &settings.maxPoints, &one, NULL, "%u");
	settings.maxPoints = std::clamp(settings.maxPoints, minPoints, maxPoints);

	ImGui::Text("Max viewport points [100 - 20000]:");
	ImGui::SameLine();
	ImGui::HelpMarker("Max points used for a single series that will be shown in the viewport without scroling.");
	ImGui::InputScalar("##maxViewportPoints", ImGuiDataType_U32, &settings.maxViewportPoints, &one, NULL, "%u");
	settings.maxViewportPoints = std::clamp(settings.maxViewportPoints, minPoints, settings.maxPoints);

	plotHandler->setSettings(settings);
}

void Gui::drawPreferencesWindow()
{
	if (showPreferencesWindow)
		ImGui::OpenPopup("Preferences");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500, 250));
	if (ImGui::BeginPopupModal("Preferences", &showPreferencesWindow, 0))
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::DragFloat("font size", &io.FontGlobalScale, 0.005f, 0.8f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

		const float buttonHeight = 25.0f;
		ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));
		if (ImGui::Button("Done", ImVec2(-1, buttonHeight)))
		{
			showPreferencesWindow = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void Gui::drawStatisticsAnalog(std::shared_ptr<Plot> plt)
{
	std::vector<std::string> serNames{"OFF"};
	for (auto& [name, ser] : plt->getSeriesMap())
		serNames.push_back(name);

	ImGui::Text("statistics ");
	ImGui::SameLine();
	ImGui::Combo("##stats", &plt->statisticsSeries, serNames);

	if (plt->statisticsSeries != 0)
	{
		static bool selectRange = false;
		ImGui::Begin("Statistics");

		auto ser = plt->getSeries(serNames[plt->statisticsSeries]);

		ImGui::ColorEdit4("##", &ser->var->getColor().r, ImGuiColorEditFlags_NoInputs);
		ImGui::SameLine();
		ImGui::Text("%s", ser->var->getName().c_str());

		ImGui::Text("select range: ");
		ImGui::SameLine();
		ImGui::Checkbox("##selectrange", &selectRange);

		plt->stats.setState(selectRange);

		Statistics::AnalogResults results;
		Statistics::calculateResults(ser.get(), &plt->getTimeSeries(), plt->stats.getValueX0(), plt->stats.getValueX1(), results);

		drawDescriptionWithNumber("t0:      ", plt->stats.getValueX0());
		drawDescriptionWithNumber("t1:      ", plt->stats.getValueX1());
		drawDescriptionWithNumber("t1-t0:   ", plt->stats.getValueX1() - plt->stats.getValueX0());
		drawDescriptionWithNumber("min:     ", results.min);
		drawDescriptionWithNumber("max:     ", results.max);
		drawDescriptionWithNumber("mean:    ", results.mean);
		drawDescriptionWithNumber("stddev:  ", results.stddev);
		ImGui::End();
	}
	else
		plt->stats.setState(false);
}

void Gui::drawStatisticsDigital(std::shared_ptr<Plot> plt)
{
	std::vector<std::string> serNames{"OFF"};
	for (auto& [name, ser] : plt->getSeriesMap())
		serNames.push_back(name);

	ImGui::Text("statistics ");
	ImGui::SameLine();
	ImGui::Combo("##stats", &plt->statisticsSeries, serNames);

	if (plt->statisticsSeries != 0)
	{
		static bool selectRange = false;
		ImGui::Begin("Statistics");

		auto ser = plt->getSeries(serNames[plt->statisticsSeries]);

		ImGui::ColorEdit4("##", &ser->var->getColor().r, ImGuiColorEditFlags_NoInputs);
		ImGui::SameLine();
		ImGui::Text("%s", ser->var->getName().c_str());

		ImGui::Text("select range: ");
		ImGui::SameLine();
		ImGui::Checkbox("##selectrange", &selectRange);

		plt->stats.setState(selectRange);

		Statistics::DigitalResults results;
		Statistics::calculateResults(ser.get(), &plt->getTimeSeries(), plt->stats.getValueX0(), plt->stats.getValueX1(), results);

		drawDescriptionWithNumber("t0:      ", plt->stats.getValueX0());
		drawDescriptionWithNumber("t1:      ", plt->stats.getValueX1());
		drawDescriptionWithNumber("t1-t0:   ", plt->stats.getValueX1() - plt->stats.getValueX0());
		drawDescriptionWithNumber("Lmin:    ", results.Lmin);
		drawDescriptionWithNumber("Lmax:    ", results.Lmax);
		drawDescriptionWithNumber("Hmin:    ", results.Hmin);
		drawDescriptionWithNumber("Hmax:    ", results.Hmax);
		drawDescriptionWithNumber("fmin:    ", results.fmin);
		drawDescriptionWithNumber("fmax:    ", results.fmax);
		ImGui::End();
	}
	else
		plt->stats.setState(false);
}

void Gui::acqusitionSettingsTrace()
{
	TracePlotHandler::Settings settings = tracePlotHandler->getSettings();

	static int one = 1;
	ImGui::Text("Max points [100 - 20000]:");
	ImGui::SameLine();
	ImGui::HelpMarker("Max points used for a single series after which the oldest points will be overwritten.");
	ImGui::InputScalar("##maxPoints", ImGuiDataType_U32, &settings.maxPoints, &one, NULL, "%u");
	settings.maxPoints = std::clamp(settings.maxPoints, static_cast<uint32_t>(100), static_cast<uint32_t>(20000));

	ImGui::Text("Viewport width in percent [0 - 100]:");
	ImGui::SameLine();
	ImGui::HelpMarker("The percentage of trace time visible during collect. Expressed in percent since the sample period is not constant.");
	ImGui::InputScalar("##maxViewportPoints", ImGuiDataType_U32, &settings.maxViewportPointsPercent, &one, NULL, "%u");
	settings.maxViewportPointsPercent = std::clamp(settings.maxViewportPointsPercent, static_cast<uint32_t>(1), static_cast<uint32_t>(100));

	ImGui::Text("Timeout [s]:");
	ImGui::SameLine();
	ImGui::HelpMarker("Timeout is the period after which trace will be stopped due to no trace data being received.");
	ImGui::InputScalar("##timeout", ImGuiDataType_U32, &settings.timeout, &one, NULL, "%u");
	settings.timeout = std::clamp(settings.timeout, static_cast<uint32_t>(1), static_cast<uint32_t>(999999));

	tracePlotHandler->setSettings(settings);
}

std::optional<std::string> Gui::showDeletePopup(const char* text, const std::string& name)
{
	ImGui::PushID(name.c_str());
	if (ImGui::BeginPopupContextItem(text))
	{
		if (ImGui::Button(text))
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			ImGui::PopID();
			return name;
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
	return std::nullopt;
}

void Gui::showQuestionBox(const char* id, const char* question, std::function<void()> onYes, std::function<void()> onNo, std::function<void()> onCancel)
{
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(id, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("%s", question);
		ImGui::Separator();
		if (ImGui::Button("Yes", ImVec2(120, 0)))
		{
			onYes();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(120, 0)))
		{
			onNo();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			onCancel();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void Gui::askShouldSaveOnExit(bool shouldOpenPopup)
{
	if (shouldOpenPopup)
		ImGui::OpenPopup("Save?");

	auto onYes = [&]()
	{
		done = true;
		if (!saveProject())
			saveProjectAs();
	};

	auto onNo = [&]()
	{ done = true; };
	auto onCancel = [&]()
	{ done = false; };

	if (vars.empty() && projectElfPath.empty() && shouldOpenPopup)
		done = true;

	showQuestionBox("Save?", "Do you want to save the current config?\n", onYes, onNo, onCancel);
}

void Gui::askShouldSaveOnNew(bool shouldOpenPopup)
{
	auto onNo = [&]()
	{
		vars.clear();
		plotHandler->removeAllPlots();
		tracePlotHandler->initPlots();
		projectElfPath = "";
		projectConfigPath = "";
	};

	if (vars.empty() && projectElfPath.empty() && shouldOpenPopup)
		onNo();
	else if (shouldOpenPopup)
		ImGui::OpenPopup("SaveOnNew?");

	auto onYes = [&]()
	{
		if (!saveProject())
			saveProjectAs();
		onNo();
	};

	showQuestionBox("SaveOnNew?", "Do you want to save the current config?\n", onYes, onNo, []() {});
}

bool Gui::saveProject()
{
	if (!projectConfigPath.empty())
		return configHandler->saveConfigFile(vars, projectElfPath, "");
	return false;
}

bool Gui::saveProjectAs()
{
	std::string path = fileHandler->saveFile(std::pair<std::string, std::string>("Project files", "cfg"));
	if (path != "")
	{
		projectConfigPath = path;
		configHandler->saveConfigFile(vars, projectElfPath, projectConfigPath);
		logger->info("Project config path: {}", projectConfigPath);
		return true;
	}
	return false;
}

bool Gui::openProject()
{
	std::string path = fileHandler->openFile(std::pair<std::string, std::string>("Project files", "cfg"));
	if (path != "")
	{
		projectConfigPath = path;
		configHandler->changeConfigFile(projectConfigPath);
		vars.clear();
		plotHandler->removeAllPlots();
		configHandler->readConfigFile(vars, projectElfPath);
		logger->info("Project config path: {}", projectConfigPath);
		return true;
	}
	return false;
}

bool Gui::openElfFile()
{
	std::string path = fileHandler->openFile(std::pair<std::string, std::string>("Elf files", "elf"));

	if (path.find(" ") != std::string::npos)
	{
		acqusitionErrorPopup.show("Error!", "Selected path contains spaces!", 2.0f);
		projectElfPath = "";
		return false;
	}

	if (path != "")
	{
		projectElfPath = path;
		logger->info("Project elf file path: {}", projectElfPath);
		return true;
	}
	return false;
}

void Gui::checkShortcuts()
{
	const ImGuiIO& io = ImGui::GetIO();
	bool wasSaved = false;

	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
		openProject();
	else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
	{
		wasSaved = saveProject();
		if (!wasSaved)
			saveProjectAs();

		popup.show("Info", "Saving successful!", 0.65f);
	}
}

void Gui::showChangeFormatPopup(const char* text, Plot& plt, const std::string& name)
{
	int format = static_cast<int>(plt.getSeriesDisplayFormat(name));

	if (plt.getSeries(name)->var->getType() == Variable::type::F32)
		return;

	if (ImGui::BeginPopupContextItem(name.c_str()))
	{
		if (ImGui::RadioButton("dec", &format, 0))
			ImGui::CloseCurrentPopup();
		if (ImGui::RadioButton("hex", &format, 1))
			ImGui::CloseCurrentPopup();
		if (ImGui::RadioButton("bin", &format, 2))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	plt.setSeriesDisplayFormat(name, static_cast<Plot::displayFormat>(format));
}

std::string Gui::intToHexString(uint32_t var)
{
	std::stringstream ss;
	ss << std::hex << var;
	return ss.str();
}

void Gui::drawCenteredText(std::string&& text)
{
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text.c_str()).x) * 0.5f);
	ImGui::Text("%s", text.c_str());
}