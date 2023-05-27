#include "Gui.hpp"

#include <unistd.h>

#include <iostream>
#include <random>
#include <sstream>

#include "ElfReader.hpp"
#include "ImguiPlugins.hpp"
#include "VarReader.hpp"
#include "glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "nfd.h"

Gui::Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, bool& done, std::mutex* mtx) : plotHandler(plotHandler), configHandler(configHandler), done(done), mtx(mtx)
{
	elfReader = std::make_unique<ElfReader>(projectElfPath);
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
	glfwSwapInterval(1);  // Enable vsync

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

	NFD_Init();

	bool show_demo_window = false;

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

		ImGui::Begin("Plots");
		drawAcqusitionSettingsWindow();
		drawPlots();
		drawMenu();
		ImGui::End();

		ImGui::Begin("VarViewer");
		drawStartButton();
		drawVarTable();
		drawPlotsTree();
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
	NFD_Quit();
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
		{
			nfdchar_t* outPath;
			nfdfilteritem_t filterItem[1] = {{"Project files", "cfg"}};
			nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
			if (result == NFD_OKAY)
			{
				projectConfigPath = std::string(outPath);
				NFD_FreePath(outPath);
				configHandler->changeConfigFile(projectConfigPath);
				vars.clear();
				plotHandler->removeAllPlots();
				projectElfPath = configHandler->getElfFilePath();
				configHandler->readConfigFile(vars, projectElfPath);
				std::replace(projectElfPath.begin(), projectElfPath.end(), '\\', '/');
				std::cout << projectConfigPath << std::endl;
			}
			else if (result == NFD_ERROR)
			{
				std::cout << "Error: %s\n"
						  << NFD_GetError() << std::endl;
			}
		}
		if (ImGui::MenuItem("Save", "Ctrl+S", false, (!projectConfigPath.empty())))
			configHandler->saveConfigFile(vars, projectElfPath, "");

		if (ImGui::MenuItem("Save As.."))
			saveAs();

		if (ImGui::MenuItem("Quit"))
			shouldSaveOnClose = true;

		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Options"))
	{
		ImGui::MenuItem("Acqusition settings...", NULL, &showAcqusitionSettingsWindow);
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
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}

	if (ImGui::Button(viewerStateMap.at(state).c_str(), ImVec2(-1, 50)))
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
		newVar->setAddress(0x20000000);
		newVar->setType(Variable::type::U8);
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dist(0, UINT32_MAX);
		newVar->setColor(static_cast<uint32_t>(dist(gen)));
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
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Variables").x) * 0.5f);
	ImGui::Text("Variables");
	ImGui::Separator();

	drawAddVariableButton();
	drawUpdateAddressesFromElf();

	if (ImGui::BeginTable("table_scrolly", 3, flags, ImVec2(0.0f, 300)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None);
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

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload("MY_DND", &var->getName(), sizeof(var->getName()));
				ImPlot::ItemIcon(var->getColorU32());
				ImGui::SameLine();
				ImGui::TextUnformatted(var->getName().c_str());
				ImGui::EndDragDropSource();
			}
			ImGui::TableSetColumnIndex(1);
			ImGui::Text(("0x" + std::string(intToHexString(var->getAddress()))).c_str());
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

void Gui::drawPlotsTree()
{
	const uint32_t windowHeight = 300;
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
	ImGui::BeginChild("left pane", ImVec2(120, -1), true);

	for (std::shared_ptr<Plot> plt : *plotHandler)
	{
		std::string name = plt->getName();
		if (ImGui::Selectable(name.c_str(), selected == name))
			selected = name;

		if (!plotNameToDelete.has_value())
			plotNameToDelete = showDeletePopup("Delete plot", name);
	}

	ImGui::EndChild();
	ImGui::SameLine();

	std::shared_ptr<Plot> plt = plotHandler->getPlot(selected);
	std::string newName = plt->getName();
	int32_t typeCombo = (int32_t)plt->getType();
	ImGui::BeginGroup();
	ImGui::Text("name    ");
	ImGui::SameLine();
	ImGui::PushID("input");
	ImGui::InputText("##", &newName, 0, NULL, NULL);
	ImGui::PopID();
	ImGui::Text("type    ");
	ImGui::SameLine();
	ImGui::PushID("combo");
	ImGui::Combo("##", &typeCombo, plotTypes, IM_ARRAYSIZE(plotTypes));
	ImGui::PopID();
	ImGui::Text("visible ");
	ImGui::SameLine();
	ImGui::Checkbox("##", &plt->getVisibilityVar());
	ImGui::PushID("list");
	if (ImGui::BeginListBox("##", ImVec2(-1, -1)))
	{
		std::optional<std::string> seriesNameToDelete = {};
		for (auto& [name, ser] : plt->getSeriesMap())
		{
			ImGui::PushID(name.c_str());
			ImGui::Checkbox("", &ser->visible);
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::Selectable(name.c_str());
			if (!seriesNameToDelete.has_value())
				seriesNameToDelete = showDeletePopup("Delete var", name);
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
	if (ImGui::BeginPopupModal("Acqusition Settings", NULL, 0))
	{
		ImGui::Text("Please pick *.elf file");
		ImGui::InputText("##", &projectElfPath, 0, NULL, NULL);
		ImGui::SameLine();
		if (ImGui::SmallButton("..."))
		{
			nfdchar_t* outPath;
			nfdfilteritem_t filterItem[1] = {{"Executable files", "elf"}};
			nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
			if (result == NFD_OKAY)
			{
				std::cout << outPath << std::endl;
				projectElfPath = std::string(outPath);
				std::replace(projectElfPath.begin(), projectElfPath.end(), '\\', '/');
				NFD_FreePath(outPath);
			}
			else if (result == NFD_ERROR)
			{
				std::cout << "Error: %s\n"
						  << NFD_GetError() << std::endl;
			}
		}

		if (ImGui::Button("Done"))
		{
			showAcqusitionSettingsWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Gui::drawPlots()
{
	uint32_t tablePlots = 0;

	for (std::shared_ptr<Plot> plt : *plotHandler)
	{
		if (plt->getType() == Plot::type_E::TABLE)
		{
			drawPlotTable(plt.get(), plt->getTimeSeries(), plt->getSeriesMap());
			if (plt->getVisibility())
				tablePlots++;
		}
	}

	uint32_t curveBarPlotsCnt = plotHandler->getVisiblePlotsCount() - tablePlots;
	uint32_t row = curveBarPlotsCnt > 0 ? curveBarPlotsCnt : 1;

	if (ImPlot::BeginSubplots("##subplos", row, 1, ImVec2(-1, -1), 0))
	{
		for (std::shared_ptr<Plot> plt : *plotHandler)
			if (plt->getType() == Plot::type_E::CURVE || plt->getType() == Plot::type_E::BAR)
				drawPlotCurveBar(plt.get(), plt->getTimeSeries(), plt->getSeriesMap(), tablePlots);
		ImPlot::EndSubplots();
	}
}

void Gui::drawPlotCurveBar(Plot* plot, ScrollingBuffer<float>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots)
{
	if (!plot->getVisibility())
		return;

	ImVec2 plotSize = ImVec2(-1, -1);

	if (plot->getType() == Plot::type_E::CURVE)
	{
		if (ImPlot::BeginPlot(plot->getName().c_str(), plotSize, ImPlotFlags_NoChild))
		{
			if (plotHandler->getViewerState() == PlotHandler::state::RUN)
				ImPlot::SetupAxes("time[s]", NULL, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
			else
			{
				ImPlot::SetupAxes("time[s]", NULL, 0, 0);
				ImPlot::SetupAxisLimits(ImAxis_X1, -1, 10, ImPlotCond_Once);
				ImPlot::SetupAxisLimits(ImAxis_Y1, -0.1, 0.1, ImPlotCond_Once);
			}

			if (ImPlot::BeginDragDropTargetPlot())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
					plot->addSeries(*vars[*(std::string*)payload->Data]);

				ImPlot::EndDragDropTarget();
			}

			/* make thread safe copies of buffers - probably can be made better but it works */
			mtx->lock();
			time.copyData();
			for (auto& [key, serPtr] : seriesMap)
			{
				if (!serPtr->visible)
					continue;
				serPtr->buffer->copyData();
			}
			uint32_t offset = time.getOffset();
			uint32_t size = time.getSize();
			mtx->unlock();

			for (auto& [key, serPtr] : seriesMap)
			{
				if (!serPtr->visible)
					continue;
				ImPlot::SetNextLineStyle(ImVec4(serPtr->var->getColor().r, serPtr->var->getColor().g, serPtr->var->getColor().b, 1.0f));
				ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
				ImPlot::PlotLine(serPtr->var->getName().c_str(), time.getFirstElementCopy(), serPtr->buffer->getFirstElementCopy(), size, 0, offset, sizeof(float));
			}

			ImPlot::EndPlot();
		}
	}
	else if (plot->getType() == Plot::type_E::BAR)
	{
		if (ImPlot::BeginPlot(plot->getName().c_str(), plotSize, ImPlotFlags_NoChild))
		{
			std::vector<const char*> glabels;
			std::vector<double> positions;

			float pos = 0.0f;
			for (const auto& [key, series] : seriesMap)
			{
				glabels.push_back(series->var->getName().c_str());
				positions.push_back(pos);
				pos += 1.0f;
			}
			glabels.push_back(nullptr);

			ImPlot::SetupAxes(NULL, "Value", 0, 0);
			ImPlot::SetupAxisLimits(ImAxis_X1, -1, seriesMap.size(), ImPlotCond_Always);
			ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), seriesMap.size(), glabels.data());

			if (ImPlot::BeginDragDropTargetPlot())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
					plot->addSeries(*vars[*(std::string*)payload->Data]);
				ImPlot::EndDragDropTarget();
			}

			float xs = 0.0f;
			float barSize = 0.5f;

			for (auto& [key, serPtr] : seriesMap)
			{
				if (!serPtr->visible)
					continue;
				float value = *serPtr->buffer->getLastElement();

				ImPlot::SetNextLineStyle(ImVec4(serPtr->var->getColor().r, serPtr->var->getColor().g, serPtr->var->getColor().b, 1.0f));
				ImPlot::PlotBars(serPtr->var->getName().c_str(), &xs, &value, 1, barSize);
				float textX = xs - barSize / 4.0f;
				float textY = value / 2.0f;
				ImPlot::Annotation(textX, textY, ImPlot::GetLastItemColor(), ImVec2(0.5f, 0.5f), false, "%.5f", value);
				xs += 1.0f;
			}
			ImPlot::EndPlot();
		}
	}
}

void Gui::drawPlotTable(Plot* plot, ScrollingBuffer<float>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap)
{
	if (!plot->getVisibility())
		return;

	static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(plot->getName().c_str()).x) * 0.5f);
	ImGui::Text(plot->getName().c_str());

	if (ImGui::BeginTable(plot->getName().c_str(), 4, flags))
	{
		ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Read value", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Write value", ImGuiTableColumnFlags_None);
		ImGui::TableHeadersRow();

		for (auto& [key, serPtr] : seriesMap)
		{
			if (!serPtr->visible)
				continue;
			float value = serPtr->var->getValue<float>();
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			Variable::Color a = serPtr->var->getColor();
			ImVec4 col = {a.r, a.g, a.b, a.a};
			ImGui::ColorButton("##", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip, ImVec2(10, 10));
			ImGui::SameLine();
			ImGui::Text(key.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::Text(("0x" + std::string(intToHexString(serPtr->var->getAddress()))).c_str());
			ImGui::TableSetColumnIndex(2);
			ImGui::SelectableInput(key.c_str(), false, ImGuiSelectableFlags_None, plot->getSeriesValueString(key, value).data(), maxVariableNameLength);
			showChangeFormatPopup("format", *plot, key);
			ImGui::TableSetColumnIndex(3);
			ImGui::PushID("input");
			char newValue[maxVariableNameLength] = {0};
			if (ImGui::SelectableInput(key.c_str(), false, ImGuiSelectableFlags_None, newValue, maxVariableNameLength))
			{
				if (plotHandler->getViewerState() == PlotHandler::state::STOP)
				{
					ImGui::PopID();
					continue;
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
				{
					std::cout << "VALUE:" << atof(newValue) << std::endl;
					if (!plotHandler->writeSeriesValue(*serPtr->var, std::stof(newValue)))
						std::cout << "ERROR while writing new value!" << std::endl;
				}
			}
			ImGui::PopID();
		}
		ImGui::EndTable();

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
				plot->addSeries(*vars[*(std::string*)payload->Data]);
			ImGui::EndDragDropTarget();
		}
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
		configHandler->saveConfigFile(vars, projectElfPath, "");
	};

	auto onNo = [&]()
	{ done = true; };
	auto onCancel = [&]()
	{ done = false; };

	showQuestionBox("Save?", "Do you want to save the current config?\n", onYes, onNo, onCancel);
}

void Gui::askShouldSaveOnNew(bool shouldOpenPopup)
{
	if (shouldOpenPopup)
		ImGui::OpenPopup("SaveOnNew?");

	auto onYes = [&]()
	{
		if (!projectConfigPath.empty())
			configHandler->saveConfigFile(vars, projectElfPath, "");
		else
			saveAs();

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

	showQuestionBox("SaveOnNew?", "Do you want to save the current config?\n", onYes, onNo, []() {});
}

void Gui::saveAs()
{
	nfdchar_t* outPath = nullptr;
	nfdfilteritem_t filterItem[1] = {{"Project files", "cfg"}};
	nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, NULL);
	if (result == NFD_OKAY)
	{
		projectConfigPath = std::string(outPath);
		configHandler->saveConfigFile(vars, projectElfPath, std::string(outPath));
		NFD_FreePath(outPath);
	}
	else if (result == NFD_ERROR)
	{
		std::cout << "Error: %s\n"
				  << NFD_GetError() << std::endl;
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