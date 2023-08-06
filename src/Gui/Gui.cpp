#include "Gui.hpp"

#include <unistd.h>

#include <random>
#include <sstream>

#include "ElfReader.hpp"
#include "glfw3.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Gui::Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, IFileHandler* fileHandler, TracePlotHandler* tracePlotHandler, bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger) : plotHandler(plotHandler), configHandler(configHandler), fileHandler(fileHandler), tracePlotHandler(tracePlotHandler), done(done), mtx(mtx), logger(logger)
{
	elfReader = std::make_unique<ElfReader>(projectElfPath, logger);
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
	glfwSwapInterval(2);  // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImPlot::StyleColorsDark();

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	fileHandler->init();

	bool show_demo_window = true;

	while (!done)
	{
		glfwSetWindowTitle(window, (std::string("STMViewer - ") + projectConfigPath).c_str());
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		if (show_demo_window)
			ImPlot::ShowDemoWindow();

		askShouldSaveOnExit(glfwWindowShouldClose(window));
		glfwSetWindowShouldClose(window, done);
		checkShortcuts();

		drawMenu();

		if (ImGui::Begin("SWO Viewer"))
		{
			drawStartButtonSwo();
			drawPlotsTreeSwo();
			ImGui::Begin("SWO Plots");
			drawPlotsSwo();
			ImGui::End();
		}
		ImGui::End();

		if (ImGui::Begin("VarViewer"))
		{
			drawStartButton();
			drawVarTable();
			drawPlotsTree();
			if (ImGui::Begin("Plots"))
			{
				drawAcqusitionSettingsWindow();
				drawPlots();
			}
			ImGui::End();
		}
		ImGui::End();

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
		ImGui::MenuItem("Acqusition settings...", NULL, &showAcqusitionSettingsWindow, plotHandler->getViewerState() == PlotHandler::state::STOP);
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();

	askShouldSaveOnExit(shouldSaveOnClose);
	askShouldSaveOnNew(shouldSaveOnNew);
}

void Gui::drawStartButton()
{
	PlotHandler::state state = plotHandler->getViewerState();

	if (state == PlotHandler::state::RUN)
	{
		ImVec4 color = (ImVec4)ImColor::HSV(0.365f, 0.94f, 0.37f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}
	else if (state == PlotHandler::state::STOP)
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
		if (state == PlotHandler::state::STOP)
		{
			plotHandler->eraseAllPlotData();
			plotHandler->setViewerState(PlotHandler::state::RUN);
		}
		else
			plotHandler->setViewerState(PlotHandler::state::STOP);
	}

	ImGui::PopStyleColor(3);
}
void Gui::drawAddVariableButton()
{
	if (ImGui::Button("Add variable", ImVec2(-1, 25)))
	{
		uint32_t num = 0;
		while (vars.find(std::string(" new") + std::to_string(num)) != vars.end())
		{
			num++;
		}
		std::string newName = std::string(" new") + std::to_string(num);

		std::shared_ptr<Variable> newVar = std::make_shared<Variable>(newName);
		std::random_device rd{};
		std::mt19937 gen{rd()};
		std::uniform_int_distribution<uint32_t> dist{0, UINT32_MAX};
		uint32_t randomColor = dist(gen);
		newVar->setColor(randomColor);
		vars.emplace(newName, newVar);
	}
}
void Gui::drawUpdateAddressesFromElf()
{
	if (ImGui::Button("Update Variable addresses", ImVec2(-1, 25)))
		elfReader->updateVariableMap(vars);
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
			if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
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
				ImGui::Text(("0x" + std::string(intToHexString(var->getAddress()))).c_str());
			else
				ImGui::Text("NOT FOUND!");
			ImGui::TableSetColumnIndex(2);
			ImGui::Text(var->getTypeStr().c_str());
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
	ImGui::Text("name      ");
	ImGui::SameLine();
	ImGui::PushID(plt->getName().c_str());
	ImGui::InputText("##input", &newName, 0, NULL, NULL);
	ImGui::Text("type      ");
	ImGui::SameLine();
	ImGui::Combo("##combo", &typeCombo, plotTypes, IM_ARRAYSIZE(plotTypes));
	bool mx0 = plt->getMarkerStateX0();
	bool mx1 = plt->getMarkerStateX1();
	ImGui::Text("x0 marker ");
	ImGui::SameLine();
	ImGui::Checkbox("##mx0", &mx0);
	plt->setMarkerStateX0(mx0);
	ImGui::Text("x1 marker ");
	ImGui::SameLine();
	ImGui::Checkbox("##mx1", &mx1);
	plt->setMarkerStateX1(mx1);
	ImGui::PopID();

	ImGui::PushID("list");
	if (ImGui::BeginListBox("##", ImVec2(-1, 190)))
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
		plt->setType(static_cast<Plot::type_E>(typeCombo));

	if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && newName != plt->getName())
	{
		plotHandler->renamePlot(plt->getName(), newName);
		selected = newName;
	}

	if (plotNameToDelete.has_value())
		plotHandler->removePlot(plotNameToDelete.value_or(""));
}

void Gui::drawAcqusitionSettingsWindow()
{
	if (showAcqusitionSettingsWindow)
		ImGui::OpenPopup("Acqusition Settings");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500, 300));
	if (ImGui::BeginPopupModal("Acqusition Settings", &showAcqusitionSettingsWindow, 0))
	{
		ImGui::Text("Project's *.elf file:");
		ImGui::InputText("##", &projectElfPath, 0, NULL, NULL);
		ImGui::SameLine();
		if (ImGui::SmallButton("..."))
			openElfFile();

		ImGui::Text("Sample period [ms]:");
		ImGui::SameLine();
		ImGui::HelpMarker("Minimum time between two respective sampling points. Set to zero for maximum frequency.");
		static int one = 1;
		ImGui::InputScalar("##sample", ImGuiDataType_U32, &settings.samplePeriod, &one, NULL, "%u");
		plotHandler->setSamplePeriod(settings.samplePeriod);

		ImGui::Text("Max points [100 - 20000]:");
		ImGui::SameLine();
		ImGui::HelpMarker("Max points used for a single series after which the oldest points will be overwritten.");
		ImGui::InputScalar("##maxPoints", ImGuiDataType_U32, &settings.maxPoints, &one, NULL, "%u");
		plotHandler->setMaxPoints(settings.maxPoints);

		ImGui::Text("Max viewport points [100 - 20000]:");
		ImGui::SameLine();
		ImGui::HelpMarker("Max points used for a single series that will be shown in the viewport without scroling.");
		ImGui::InputScalar("##maxViewportPoints", ImGuiDataType_U32, &settings.maxViewportPoints, &one, NULL, "%u");

		if (ImGui::Button("Done"))
		{
			showAcqusitionSettingsWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

std::optional<std::string> Gui::showDeletePopup(const char* text, const std::string name)
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
		ImGui::Text(question);
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
	if (shouldOpenPopup)
		ImGui::OpenPopup("SaveOnNew?");

	auto onYes = [&]()
	{
		if (!saveProject())
			saveProjectAs();
		vars.clear();
		plotHandler->removeAllPlots();
		projectElfPath = "";
		projectConfigPath = "";
	};

	auto onNo = [&]()
	{
		vars.clear();
		plotHandler->removeAllPlots();
		projectElfPath = "";
		projectConfigPath = "";
	};

	if (vars.empty() && projectElfPath.empty() && shouldOpenPopup)
		onYes();

	showQuestionBox("SaveOnNew?", "Do you want to save the current config?\n", onYes, onNo, []() {});
}

bool Gui::saveProject()
{
	if (!projectConfigPath.empty())
		return configHandler->saveConfigFile(vars, projectElfPath, settings, "");
	return false;
}

bool Gui::saveProjectAs()
{
	std::string path = fileHandler->saveFile(std::pair<std::string, std::string>("Project files", "cfg"));
	if (path != "")
	{
		projectConfigPath = path;
		configHandler->saveConfigFile(vars, projectElfPath, settings, projectConfigPath);
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
		configHandler->readConfigFile(vars, projectElfPath, settings);
		plotHandler->setSamplePeriod(settings.samplePeriod);
		plotHandler->setMaxPoints(settings.maxPoints);
		logger->info("Project config path: {}", projectConfigPath);
		return true;
	}
	return false;
}

bool Gui::openElfFile()
{
	std::string path = fileHandler->openFile(std::pair<std::string, std::string>("Elf files", "elf"));
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
	ImGuiIO& io = ImGui::GetIO();

	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
		openProject();
	else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
	{
		if (!saveProject())
			saveProjectAs();
	}
}

void Gui::showChangeFormatPopup(const char* text, Plot& plt, const std::string& name)
{
	int format = static_cast<int>(plt.getSeriesDisplayFormat(name));

	if (plt.getSeries(name)->var->getType() == Variable::type::F32)
		return;

	if (ImGui::BeginPopupContextItem())
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