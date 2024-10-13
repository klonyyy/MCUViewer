#include "Gui.hpp"

#include <imgui.h>
#include <unistd.h>

#include <future>
#include <random>
#include <set>
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

Gui::Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, IFileHandler* fileHandler, TracePlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, GdbParser* parser, spdlog::logger* logger, std::string& projectPath) : plotHandler(plotHandler), configHandler(configHandler), fileHandler(fileHandler), tracePlotHandler(tracePlotHandler), done(done), mtx(mtx), parser(parser), logger(logger)
{
	threadHandle = std::thread(&Gui::mainThread, this, projectPath);
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

static float getContentScale(GLFWwindow* window)
{
	float xscale;
	float yscale;
	glfwGetWindowContentScale(window, &xscale, &yscale);
	return (xscale + yscale) / 2.0f;
}

void Gui::mainThread(std::string externalPath)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return;

	GLFWwindow* window = glfwCreateWindow(1500, 1000, (std::string("MCUViewer | ") + projectConfigPath).c_str(), NULL, NULL);
	if (window == NULL)
		return;
	glfwMakeContextCurrent(window);
	glfwMaximizeWindow(window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	GuiHelper::contentScale = getContentScale(window);

	ImFontConfig cfg;
	cfg.SizePixels = 13.0f * GuiHelper::contentScale;

	ImGui::GetStyle().ScaleAllSizes(GuiHelper::contentScale);

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	io.Fonts->AddFontDefault(&cfg);
	io.FontGlobalScale = 1.0f;

	ImGui::StyleColorsDark();
	ImPlot::StyleColorsDark();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

	fileHandler->init();

	jlinkProbe = std::make_shared<JlinkDebugProbe>(logger);
	stlinkProbe = std::make_shared<StlinkDebugProbe>(logger);
	debugProbeDevice = stlinkProbe;
	plotHandler->setDebugProbe(debugProbeDevice);

	jlinkTraceProbe = std::make_shared<JlinkTraceProbe>(logger);
	stlinkTraceProbe = std::make_shared<StlinkTraceProbe>(logger);
	traceProbeDevice = stlinkTraceProbe;
	tracePlotHandler->setDebugProbe(traceProbeDevice);

	if (!externalPath.empty())
		openProject(externalPath);

	while (!done)
	{
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
		{
			glfwWaitEvents();
			continue;
		}

		if (glfwGetWindowAttrib(window, GLFW_FOCUSED) || (tracePlotHandler->getViewerState() == PlotHandlerBase::state::RUN) || (plotHandler->getViewerState() == PlotHandlerBase::state::RUN))
			glfwSwapInterval(1);
		else
			glfwSwapInterval(4);

		glfwSetWindowTitle(window, (std::string("MCUViewer - ") + projectConfigPath).c_str());
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		if (showDemoWindow)
			ImPlot::ShowDemoWindow();

		if (glfwWindowShouldClose(window))
		{
			plotHandler->setViewerState(PlotHandlerBase::state::STOP);
			tracePlotHandler->setViewerState(PlotHandlerBase::state::STOP);
			askShouldSaveOnExit(glfwWindowShouldClose(window));
		}
		glfwSetWindowShouldClose(window, done);
		checkShortcuts();

		drawMenu();
		drawAboutWindow();
		drawPreferencesWindow();

		if (ImGui::Begin("Trace Viewer"))
		{
			activeView = ActiveViewType::TraceViewer;
			drawAcqusitionSettingsWindow(activeView);
			ImGui::SetNextWindowClass(&window_class);
			if (ImGui::Begin("Trace Plots"))
				drawPlotsSwo();
			ImGui::End();
			drawStartButton(tracePlotHandler);
			drawSettingsSwo();
			drawIndicatorsSwo();
			drawPlotsTreeSwo();
		}
		ImGui::End();

		if (ImGui::Begin("Var Viewer"))
		{
			activeView = ActiveViewType::VarViewer;
			drawAcqusitionSettingsWindow(activeView);
			drawStartButton(plotHandler);
			drawVarTable();
			drawPlotsTree();
			drawImportVariablesWindow();
			variableEditWindow.drawVariableEditWindow();
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

	if (activeView == ActiveViewType::VarViewer)
	{
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 210 * GuiHelper::contentScale));
		GuiHelper::drawDescriptionWithNumber("sampling: ", plotHandler->getAverageSamplingFrequency(), " Hz", 2);
	}

	ImGui::EndMainMenuBar();
	askShouldSaveOnExit(shouldSaveOnClose);
	askShouldSaveOnNew(shouldSaveOnNew);
}

void Gui::drawStartButton(PlotHandlerBase* activePlotHandler)
{
	bool shouldDisableButton = (!devicesList.empty() && devicesList.front() == noDevices);
	ImGui::BeginDisabled(shouldDisableButton);

	PlotHandlerBase::state state = activePlotHandler->getViewerState();

	if (state == PlotHandlerBase::state::RUN)
	{
		ImVec4 green = (ImVec4)ImColor::HSV(0.365f, 0.94f, 0.37f);
		ImVec4 greenLight = (ImVec4)ImColor::HSV(0.365f, 0.94f, 0.57f);
		ImVec4 greenLightDim = (ImVec4)ImColor::HSV(0.365f, 0.94f, 0.47f);

		ImGui::PushStyleColor(ImGuiCol_Button, green);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, greenLight);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, greenLightDim);
	}
	else if (state == PlotHandlerBase::state::STOP)
	{
		if (activePlotHandler->getLastReaderError() != "")
		{
			ImVec4 red = (ImVec4)ImColor::HSV(0.0f, 0.95f, 0.72f);
			ImVec4 redLight = (ImVec4)ImColor::HSV(0.0f, 0.95f, 0.92f);
			ImVec4 redLightDim = (ImVec4)ImColor::HSV(0.0f, 0.95f, 0.82f);

			ImGui::PushStyleColor(ImGuiCol_Button, red);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redLight);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, redLightDim);
		}
		else
		{
			ImVec4 orange = (ImVec4)ImColor::HSV(0.116f, 0.97f, 0.72f);
			ImVec4 orangeLight = (ImVec4)ImColor::HSV(0.116f, 0.97f, 0.92f);
			ImVec4 orangeLightDim = (ImVec4)ImColor::HSV(0.116f, 0.97f, 0.82f);

			ImGui::PushStyleColor(ImGuiCol_Button, orange);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, orange);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, orangeLightDim);
		}
	}

	if (ImGui::Button((viewerStateMap.at(state) + " " + activePlotHandler->getLastReaderError()).c_str(), ImVec2(-1, 50 * GuiHelper::contentScale)) || (ImGui::IsKeyPressed(ImGuiKey_Space, false) && !shouldDisableButton))
	{
		if (state == PlotHandlerBase::state::STOP)
		{
			logger->info("Start clicked!");
			activePlotHandler->eraseAllPlotData();
			activePlotHandler->setViewerState(PlotHandlerBase::state::RUN);
		}
		else
		{
			logger->info("Stop clicked!");
			activePlotHandler->setViewerState(PlotHandlerBase::state::STOP);
		}
	}

	ImGui::PopStyleColor(3);
	ImGui::EndDisabled();
}

void Gui::addNewVariable(const std::string& newName)
{
	std::shared_ptr<Variable> newVar = std::make_shared<Variable>(newName);
	std::random_device rd{};
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<uint32_t> dist{0, UINT32_MAX};
	uint32_t randomColor = dist(gen);
	newVar->setColor(randomColor);
	newVar->renameCallback = [&](const std::string& currentName, const std::string& newName)
	{
		renameVariable(currentName, newName);
	};
	vars.emplace(newName, newVar);
}

void Gui::drawAddVariableButton()
{
	if (ImGui::Button("Add variable", ImVec2(-1, 25 * GuiHelper::contentScale)))
	{
		uint32_t num = 0;
		while (vars.find(std::string("-new") + std::to_string(num)) != vars.end())
			num++;

		addNewVariable(std::string("-new") + std::to_string(num));
	}

	ImGui::BeginDisabled(projectElfPath.empty());

	if (ImGui::Button("Import variables from *.elf", ImVec2(-1, 25 * GuiHelper::contentScale)))
		showImportVariablesWindow = true;

	ImGui::EndDisabled();
}

void Gui::drawUpdateAddressesFromElf()
{
	static std::future<bool> refreshThread{};
	static bool shouldPopStyle = false;

	static constexpr size_t textSize = 40;
	char buttonText[textSize]{};

	if (refreshThread.valid() && refreshThread.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		snprintf(buttonText, textSize, "Update variable addresses %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
	else
	{
		snprintf(buttonText, textSize, "Update variable addresses");
		if (refreshThread.valid() && !refreshThread.get())
			popup.show("Error!", "Update error. Please check the *.elf file path!", 2.0f);
	}

	ImGui::BeginDisabled(projectElfPath.empty());

	bool elfChanged = checkElfFileChanged();

	if (elfChanged)
	{
		ImVec4 color = ImColor::HSV(0.1f, 0.97f, 0.72f);
		snprintf(buttonText, textSize, "Click to reload *.elf changes!");
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

		if (plotHandler->getSettings().stopAcqusitionOnElfChange)
			plotHandler->setViewerState(PlotHandlerBase::state::STOP);

		if (plotHandler->getSettings().refreshAddressesOnElfChange)
		{
			performVariablesUpdate = true;
			/* TODO: examine why elf is not ready to be parsed without the delay */
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		shouldPopStyle = true;
	}

	if (ImGui::Button(buttonText, ImVec2(-1, 25 * GuiHelper::contentScale)) || performVariablesUpdate)
	{
		lastModifiedTime = std::filesystem::file_time_type::clock::now();
		refreshThread = std::async(std::launch::async, &GdbParser::updateVariableMap2, parser, this->convertProjectPathToAbsolute(projectElfPath), std::ref(vars));
		performVariablesUpdate = false;
	}

	/* TODO fix this ugly solution */
	if (shouldPopStyle)
	{
		ImGui::PopStyleColor(3);
		shouldPopStyle = false;
	}
	ImGui::EndDisabled();
}

void Gui::drawVarTable()
{
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;
	static std::set<std::string> selection;

	ImGui::BeginDisabled(plotHandler->getViewerState() == PlotHandlerBase::state::RUN);
	ImGui::Dummy(ImVec2(-1, 5));
	GuiHelper::drawCenteredText("Variables");
	ImGui::SameLine();
	ImGui::HelpMarker("Select your *.elf file in the Options->Acqusition Settings to import or update the variables.");
	ImGui::Separator();

	drawAddVariableButton();
	drawUpdateAddressesFromElf();

	const char* label = "search ";
	ImGui::PushItemWidth(ImGui::GetItemRectSize().x - ImGui::CalcTextSize(label).x - 8 * GuiHelper::contentScale);
	static std::string search{};
	ImGui::Text("%s", label);
	ImGui::SameLine();
	ImGui::InputText("##search", &search, 0, NULL, NULL);
	ImGui::PopItemWidth();

	if (ImGui::BeginTable("table_scrolly", 3, flags, ImVec2(0.0f, 300 * GuiHelper::contentScale)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name", 0);
		ImGui::TableSetupColumn("Address", 0);
		ImGui::TableSetupColumn("Type", 0);
		ImGui::TableHeadersRow();

		std::optional<std::string> varNameToDelete;
		std::optional<std::pair<std::string, std::string>> varNameToRename;

		std::string currentName{};

		for (auto& [name, var] : vars)
		{
			if (toLower(name).find(toLower(search)) == std::string::npos)
				continue;

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(name.c_str());
			ImGui::ColorEdit4("##", &var->getColor().r, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::PopID();
			char variable[maxVariableNameLength] = {0};
			std::memcpy(variable, var->getName().data(), var->getName().length());

			const bool itemIsSelected = selection.contains(name);

			if (ImGui::Selectable(var->getName().c_str(), itemIsSelected, ImGuiSelectableFlags_AllowDoubleClick))
			{
				if (ImGui::IsMouseDoubleClicked(0))
				{
					variableEditWindow.setVariableToEdit(var);
					variableEditWindow.setShowVariableEditWindowState(true);
				}

				if (ImGui::GetIO().KeyCtrl && var->getIsFound())
				{
					if (itemIsSelected)
						selection.erase(name);
					else
						selection.insert(name);
				}
				else
					selection.clear();

				if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
					varNameToRename = {name, std::string(variable)};
			}

			if (!varNameToDelete.has_value())
				varNameToDelete = showDeletePopup("Delete", name);

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				if (selection.empty())
					selection.insert(name);

				/* pass a pointer to the selection as we have to call clear() on the original object upon receiving */
				std::set<std::string>* selectionPtr = &selection;
				ImGui::SetDragDropPayload("MY_DND", &selectionPtr, sizeof(selectionPtr));
				ImGui::PushID(name.c_str());
				ImGui::ColorEdit4("##", &vars[*selection.begin()]->getColor().r, ImGuiColorEditFlags_NoInputs);
				ImGui::SameLine();
				ImGui::PopID();

				if (selection.size() > 1)
					ImGui::TextUnformatted("<multiple vars>");
				else
					ImGui::TextUnformatted(selection.begin()->c_str());
				ImGui::EndDragDropSource();
			}
			ImGui::TableSetColumnIndex(1);
			if (var->getIsFound() == true)
				ImGui::Text("%s", ("0x" + std::string(GuiHelper::intToHexString(var->getAddress()))).c_str());
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

		if (varNameToRename.has_value())
		{
			auto var = vars[varNameToRename.value().first];
			var->rename(varNameToRename.value().second);
		}
		ImGui::EndTable();
	}

	ImGui::EndDisabled();
}

void Gui::renameVariable(const std::string& currentName, const std::string& newName)
{
	auto temp = vars.extract(currentName);
	temp.key() = std::string(newName);
	vars.insert(std::move(temp));

	for (std::shared_ptr<Plot> plt : *plotHandler)
		plt->renameSeries(currentName, newName);
}

void Gui::drawAddPlotButton()
{
	if (ImGui::Button("Add plot", ImVec2(-1, 25 * GuiHelper::contentScale)))
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
	if (ImGui::Button("Export plot to *.csv", ImVec2(-1, 25 * GuiHelper::contentScale)))
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
	const uint32_t windowHeight = 320 * GuiHelper::contentScale;
	const char* plotTypes[3] = {"curve", "bar", "table"};
	static std::string selected = "";
	std::optional<std::string> plotNameToDelete = {};

	ImGui::Dummy(ImVec2(-1, 5));
	GuiHelper::drawCenteredText("Plots");
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
	ImGui::BeginChild("left pane", ImVec2(150 * GuiHelper::contentScale, -1), true);

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
	if (ImGui::BeginListBox("##", ImVec2(-1, 175 * GuiHelper::contentScale)))
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
		{
			std::set<std::string>* selection = *(std::set<std::string>**)payload->Data;

			for (const auto& name : *selection)
				plt->addSeries(*vars[name]);
			selection->clear();
		}
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

void Gui::drawAcqusitionSettingsWindow(ActiveViewType type)
{
	if (showAcqusitionSettingsWindow)
		ImGui::OpenPopup("Acqusition Settings");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(950 * GuiHelper::contentScale, 500 * GuiHelper::contentScale));
	if (ImGui::BeginPopupModal("Acqusition Settings", &showAcqusitionSettingsWindow, 0))
	{
		if (type == ActiveViewType::VarViewer)
			acqusitionSettingsViewer();
		else if (type == ActiveViewType::TraceViewer)
			acqusitionSettingsTrace();

		acqusitionErrorPopup.handle();

		const float buttonHeight = 25.0f * GuiHelper::contentScale;
		ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));

		if (ImGui::Button("Done", ImVec2(-1, buttonHeight)))
		{
			showAcqusitionSettingsWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Gui::drawPreferencesWindow()
{
	if (showPreferencesWindow)
		ImGui::OpenPopup("Preferences");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500 * GuiHelper::contentScale, 250 * GuiHelper::contentScale));
	if (ImGui::BeginPopupModal("Preferences", &showPreferencesWindow, 0))
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::DragFloat("font size", &io.FontGlobalScale, 0.005f, 0.8f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

		const float buttonHeight = 25.0f * GuiHelper::contentScale;
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

		GuiHelper::drawDescriptionWithNumber("t0:      ", plt->stats.getValueX0());
		GuiHelper::drawDescriptionWithNumber("t1:      ", plt->stats.getValueX1());
		GuiHelper::drawDescriptionWithNumber("t1-t0:   ", plt->stats.getValueX1() - plt->stats.getValueX0());
		GuiHelper::drawDescriptionWithNumber("min:     ", results.min);
		GuiHelper::drawDescriptionWithNumber("max:     ", results.max);
		GuiHelper::drawDescriptionWithNumber("mean:    ", results.mean);
		GuiHelper::drawDescriptionWithNumber("stddev:  ", results.stddev);
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

		GuiHelper::drawDescriptionWithNumber("t0:      ", plt->stats.getValueX0());
		GuiHelper::drawDescriptionWithNumber("t1:      ", plt->stats.getValueX1());
		GuiHelper::drawDescriptionWithNumber("t1-t0:   ", plt->stats.getValueX1() - plt->stats.getValueX0());
		GuiHelper::drawDescriptionWithNumber("Lmin:    ", results.Lmin);
		GuiHelper::drawDescriptionWithNumber("Lmax:    ", results.Lmax);
		GuiHelper::drawDescriptionWithNumber("Hmin:    ", results.Hmin);
		GuiHelper::drawDescriptionWithNumber("Hmax:    ", results.Hmax);
		GuiHelper::drawDescriptionWithNumber("fmin:    ", results.fmin);
		GuiHelper::drawDescriptionWithNumber("fmax:    ", results.fmax);
		ImGui::End();
	}
	else
		plt->stats.setState(false);
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
	float buttonWidth = 120.0f * GuiHelper::contentScale;
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(id, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("%s", question);
		ImGui::Separator();
		if (ImGui::Button("Yes", ImVec2(buttonWidth, 0)))
		{
			onYes();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(buttonWidth, 0)))
		{
			onNo();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
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

bool Gui::openProject(std::string externalPath)
{
	std::string path = "";

	if (externalPath.empty())
		path = fileHandler->openFile(std::pair<std::string, std::string>("Project files", "cfg"));
	else
		path = externalPath;

	if (path != "")
	{
		projectConfigPath = path;
		configHandler->changeConfigFile(projectConfigPath);
		vars.clear();
		plotHandler->removeAllPlots();
		configHandler->readConfigFile(vars, projectElfPath);

		/* attach rename callback so that all references are updated on variable rename */
		for (auto& [name, var] : vars)
		{
			var->renameCallback = [&](const std::string& currentName, const std::string& newName)
			{ renameVariable(currentName, newName); };
		}

		logger->info("Project config path: {}", projectConfigPath);
		/* TODO refactor */
		devicesList.clear();
		if (plotHandler->getProbeSettings().debugProbe == 1)
			debugProbeDevice = jlinkProbe;
		else
			debugProbeDevice = stlinkProbe;

		plotHandler->setDebugProbe(debugProbeDevice);

		if (tracePlotHandler->getProbeSettings().debugProbe == 1)
			traceProbeDevice = jlinkTraceProbe;
		else
			traceProbeDevice = stlinkTraceProbe;

		tracePlotHandler->setDebugProbe(traceProbeDevice);

		return true;
	}
	return false;
}

bool Gui::openElfFile()
{
	std::string path = fileHandler->openFile({"Elf files", "elf"});

	if (path.find(" ") != std::string::npos)
	{
		acqusitionErrorPopup.show("Error!", "Selected path contains spaces!", 2.0f);
		projectElfPath = "";
		return false;
	}

	if (path != "")
	{
		std::filesystem::path relPath = std::filesystem::relative(path, std::filesystem::path(projectConfigPath).parent_path());
		if (relPath != "")
			projectElfPath = relPath.string();
		else
			projectElfPath = path;

		logger->info("Project elf file path: {}", projectElfPath);
		return true;
	}
	return false;
}

bool Gui::openLogDirectory(std::string& logDirectory)
{
	std::string path = fileHandler->openDirectory({"", ""});

	if (path != "")
	{
		logDirectory = path;
		logger->info("Log directory: {}", path);
		return true;
	}
	return false;
}

std::string Gui::convertProjectPathToAbsolute(const std::string& relativePath)
{
	if (relativePath.empty())
		return "";

	try
	{
		// Convert relative path to absolute path based on project file location
		std::filesystem::path absPath = std::filesystem::absolute(std::filesystem::path(projectConfigPath).parent_path() / relativePath);
		return absPath.string();
	}
	catch (std::filesystem::filesystem_error& e)
	{
		logger->error("Failed to convert path to absolute: {}", e.what());
		return "";
	}
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

bool Gui::checkElfFileChanged()
{
	if (!std::filesystem::exists(convertProjectPathToAbsolute(projectElfPath)))
		return false;

	auto writeTime = std::filesystem::last_write_time(convertProjectPathToAbsolute(projectElfPath));
	return writeTime > lastModifiedTime;
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
