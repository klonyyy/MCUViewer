#include "Gui.hpp"

#include <unistd.h>

#include <random>
#include <sstream>

#include "ElfReader.hpp"
#include "ImguiPlugins.hpp"
#include "glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

Gui::Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, IFileHandler* fileHandler, bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger) : plotHandler(plotHandler), configHandler(configHandler), fileHandler(fileHandler), done(done), mtx(mtx), logger(logger)
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
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Variables").x) * 0.5f);
	ImGui::Text("Variables");
	ImGui::Separator();

	drawAddVariableButton();
	drawUpdateAddressesFromElf();

	if (ImGui::BeginTable("table_scrolly", 3, flags, ImVec2(0.0f, 300)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
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
	if (ImGui::BeginListBox("##", ImVec2(-1, -1)))
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
		static int one = 1;
		ImGui::InputScalar("##sample", ImGuiDataType_U32, &settings.samplePeriod, &one, NULL, "%u");
		plotHandler->setSamplePeriod(settings.samplePeriod);

		ImGui::Text("Max points [100 - 20000]:");
		ImGui::InputScalar("##maxPoints", ImGuiDataType_U32, &settings.maxPoints, &one, NULL, "%u");
		plotHandler->setMaxPoints(settings.maxPoints);

		ImGui::Text("Max viewport points [100 - 20000]:");
		ImGui::InputScalar("##maxViewportPoints", ImGuiDataType_U32, &settings.maxViewportPoints, &one, NULL, "%u");

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

	ImVec2 initialCursorPos = ImGui::GetCursorPos();

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

	const float remainingSpace = (ImGui::GetWindowPos().y + ImGui::GetWindowSize().y) - (ImGui::GetCursorPos().y + initialCursorPos.y);
	ImVec2 plotSize(-1, -1);
	if (remainingSpace < 300)
		plotSize.y = 300;

	if (ImPlot::BeginSubplots("##subplos", row, 1, plotSize, 0))
	{
		for (std::shared_ptr<Plot> plt : *plotHandler)
			if (plt->getType() == Plot::type_E::CURVE || plt->getType() == Plot::type_E::BAR)
				drawPlotCurveBar(plt.get(), plt->getTimeSeries(), plt->getSeriesMap(), tablePlots);
		ImPlot::EndSubplots();
	}
}

void Gui::drawPlotCurveBar(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots)
{
	if (!plot->getVisibility())
		return;

	ImVec2 plotSize = ImVec2(-1, -1);

	if (plot->getType() == Plot::type_E::CURVE)
	{
		if (ImPlot::BeginPlot(plot->getName().c_str(), plotSize, ImPlotFlags_NoChild))
		{
			if (plotHandler->getViewerState() == PlotHandler::state::RUN)
			{
				ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_AutoFit);
				ImPlot::SetupAxis(ImAxis_X1, "time[s]", 0);
				const double viewportWidth = (settings.samplePeriod > 0 ? settings.samplePeriod : 1) * 0.001f * settings.maxViewportPoints;
				const double min = *time.getLastElement() < viewportWidth ? 0.0f : *time.getLastElement() - viewportWidth;
				const double max = min == 0.0f ? *time.getLastElement() : min + viewportWidth;
				ImPlot::SetupAxisLimits(ImAxis_X1, min, max, ImPlotCond_Always);
			}
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

			ImPlotRect plotLimits = ImPlot::GetPlotLimits();

			if (plot->getMarkerStateX0())
			{
				double markerPos = plot->getMarkerValueX0();
				if (markerPos == 0.0)
				{
					markerPos = plotLimits.X.Min + ((std::abs(plotLimits.X.Max) - std::abs(plotLimits.X.Min)) / 3.0f);
					plot->setMarkerValueX0(markerPos);
				}
				ImPlot::DragLineX(0, &markerPos, ImVec4(1, 0, 1, 1));
				plot->setMarkerValueX0(markerPos);

				ImPlot::Annotation(markerPos, plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(-10, 0), true, "x0 %.5f", markerPos);
			}
			else
				plot->setMarkerValueX0(0.0);

			if (plot->getMarkerStateX1())
			{
				double markerPos = plot->getMarkerValueX1();
				if (markerPos == 0.0)
				{
					markerPos = plotLimits.X.Min + (2.0f * (std::abs(plotLimits.X.Max) - std::abs(plotLimits.X.Min)) / 3.0f);
					plot->setMarkerValueX1(markerPos);
				}
				ImPlot::DragLineX(1, &markerPos, ImVec4(1, 1, 0, 1));
				plot->setMarkerValueX1(markerPos);
				ImPlot::Annotation(markerPos, plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 0), true, "x1 %.5f", markerPos);
				double dx = markerPos - plot->getMarkerValueX0();
				ImPlot::Annotation(markerPos, plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 20), true, "x1-x0 %.5f", dx);
			}
			else
				plot->setMarkerValueX1(0.0);

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

				const double timepoint = plot->getMarkerValueX0();
				const double value = *(serPtr->buffer->getFirstElementCopy() + time.getIndexFromvalue(timepoint));
				auto name = plot->getMarkerStateX0() ? key + " = " + std::to_string(value) : key;

				ImPlot::SetNextLineStyle(ImVec4(serPtr->var->getColor().r, serPtr->var->getColor().g, serPtr->var->getColor().b, 1.0f));
				ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f);
				ImPlot::PlotLine(name.c_str(), time.getFirstElementCopy(), serPtr->buffer->getFirstElementCopy(), size, 0, offset, sizeof(double));

				if (plot->getMarkerStateX0())
				{
					ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f, ImVec4(255, 255, 255, 255), 0.5f);
					ImPlot::PlotScatter("###point", &timepoint, &value, 1, false);
				}
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

			double xs = 0.0f;
			double barSize = 0.5f;

			for (auto& [key, serPtr] : seriesMap)
			{
				if (!serPtr->visible)
					continue;
				double value = *serPtr->buffer->getLastElement();

				ImPlot::SetNextLineStyle(ImVec4(serPtr->var->getColor().r, serPtr->var->getColor().g, serPtr->var->getColor().b, 1.0f));
				ImPlot::PlotBars(serPtr->var->getName().c_str(), &xs, &value, 1, barSize);
				ImPlot::Annotation(xs, value / 2.0f, ImVec4(0, 0, 0, 0), ImVec2(0, -5), true, "%.5f", value);
				xs += 1.0f;
			}
			ImPlot::EndPlot();
		}
	}
}

void Gui::drawPlotTable(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap)
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
			ImGui::SelectableInput(key.c_str(), false, ImGuiSelectableFlags_None, plot->getSeriesValueString(key, serPtr->var->getValue()).data(), maxVariableNameLength);
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
					logger->info("New value to be written: {}", newValue);
					if (!plotHandler->writeSeriesValue(*serPtr->var, std::stod(newValue)))
						logger->error("Error while writing new value!");
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