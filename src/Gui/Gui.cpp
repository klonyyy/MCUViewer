#include "Gui.hpp"

#include <SDL.h>
#include <SDL_opengl.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#include "ElfReader.hpp"
#include "ImguiPlugins.hpp"
#include "VarReader.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "implot.h"
#include "nfd.h"

Gui::Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, bool& done, std::mutex* mtx) : plotHandler(plotHandler), configHandler(configHandler), done(done), mtx(mtx)
{
	elfReader = std::make_unique<ElfReader>(projectElfFile);
	threadHandle = std::thread(&Gui::mainThread, this);
}

Gui::~Gui()
{
	if (threadHandle.joinable())
		threadHandle.join();
}

void Gui::mainThread()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return;
	}
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_Window* window;

	window = SDL_CreateWindow("STMViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1500, 1000, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1);	// Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	NFD_Init();

	bool show_demo_window = true;
	bool p_open = true;

	while (!done)
	{
		SDL_Event event;
		if (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		if (show_demo_window)
			ImPlot::ShowDemoWindow();

		ImGui::Begin("Plots", &p_open, 0);
		if (showAcqusitionSettingsWindow)
			drawAcqusitionSettingsWindow();

		for (Plot* plt : *plotHandler)
			drawPlot(plt, plt->getTimeSeries(), plt->getSeriesMap());

		drawMenu();
		ImGui::End();

		ImGui::Begin("VarViewer", &p_open, 0);
		drawStartButton();
		drawAddVariableButton();
		drawUpdateAddressesFromElf();
		drawVarTable();
		drawPlotsTree();
		ImGui::End();

		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
			SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
		}

		SDL_GL_SwapWindow(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	NFD_Quit();
	SDL_Quit();
}

void Gui::drawMenu()
{
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New"))
		{
		}
		if (ImGui::MenuItem("Open", "Ctrl+O"))
		{
			nfdchar_t* outPath;
			nfdfilteritem_t filterItem[1] = {{"Project files", "cfg"}};
			nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
			if (result == NFD_OKAY)
			{
				configHandler->changeConfigFile(std::string(outPath));
				vars.clear();
				plotHandler->removeAllPlots();
				projectElfFile = configHandler->getElfFilePath();
				configHandler->readConfigFile(vars, projectElfFile);
				std::replace(projectElfFile.begin(), projectElfFile.end(), '\\', '/');
				std::cout << outPath << std::endl;
				NFD_FreePath(outPath);
			}
			else if (result == NFD_ERROR)
			{
				std::cout << "Error: %s\n"
						  << NFD_GetError() << std::endl;
			}
		}
		if (ImGui::BeginMenu("Open Recent"))
		{
			ImGui::MenuItem("fish_hat.c");
			ImGui::MenuItem("fish_hat.inl");
			ImGui::MenuItem("fish_hat.h");
			if (ImGui::BeginMenu("More.."))
			{
				ImGui::MenuItem("Hello");
				ImGui::MenuItem("Sailor");
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Save", "Ctrl+S"))
		{
			configHandler->saveConfigFile(vars, projectElfFile, projectConfigFile);
		}
		if (ImGui::MenuItem("Save As.."))
		{
			nfdchar_t* outPath;
			nfdfilteritem_t filterItem[1] = {{"Project files", "cfg"}};
			nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, NULL);
			if (result == NFD_OKAY)
			{
				configHandler->saveConfigFile(vars, projectElfFile, std::string(outPath));
				NFD_FreePath(outPath);
			}
			else if (result == NFD_ERROR)
			{
				std::cout << "Error: %s\n"
						  << NFD_GetError() << std::endl;
			}
		}
		if (ImGui::MenuItem("Quit"))
		{
			done = true;
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Options"))
	{
		ImGui::MenuItem("Acqusition settings...", NULL, &showAcqusitionSettingsWindow);
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void Gui::drawStartButton()
{
	if (viewerState == state::RUN)
	{
		ImVec4 color = (ImVec4)ImColor::HSV(0.365f, 0.94f, 0.37f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}
	else if (viewerState == state::STOP)
	{
		ImVec4 color = ImColor::HSV(0.116f, 0.97f, 0.72f);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	}

	if (ImGui::Button(viewerStateMap.at(viewerState).c_str(), ImVec2(-1, 50)))
	{
		if (viewerState == state::STOP)
		{
			viewerState = state::RUN;
			plotHandler->eraseAllPlotData();
			plotHandler->setViewerState((PlotHandler::state)state::RUN);
		}
		else
		{
			plotHandler->setViewerState((PlotHandler::state)state::STOP);
			viewerState = state::STOP;
		}
	}

	ImGui::PopStyleColor(3);
}
void Gui::drawAddVariableButton()
{
	if (ImGui::Button("Add variable", ImVec2(-1, 30)))
	{
		uint32_t num = 0;
		while (vars.find(std::string("new") + std::to_string(num)) != vars.end())
		{
			num++;
		}
		std::string newName = std::string("new") + std::to_string(num);

		std::shared_ptr<Variable> newVar = std::make_shared<Variable>(newName);
		newVar->setAddress(0x20000000);
		newVar->setType(Variable::type::U8);
		vars.emplace(newName, newVar);
	}
}
void Gui::drawUpdateAddressesFromElf()
{
	if (ImGui::Button("Update Variable addresses", ImVec2(-1, 30)))
		elfReader->updateVariableMap(vars);
}

void Gui::drawVarTable()
{
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

	if (ImGui::BeginTable("table_scrolly", 3, flags, ImVec2(0.0f, 300)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None);
		ImGui::TableHeadersRow();

		for (auto& [keyName, var] : vars)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(keyName.c_str());
			ImGui::ColorEdit4("##", &var->getColor().r, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::PopID();
			char variable[maxVariableNameLength] = {0};
			memcpy(variable, var->getName().data(), (var->getName().length()));
			ImGui::SelectableInput(var->getName().c_str(), false, ImGuiSelectableFlags_None, variable, maxVariableNameLength);
			if (ImGui::IsKeyPressed(ImGuiKey_Enter))
			{
				auto varr = vars.extract(var->getName());
				varr.key() = std::string(variable);
				var->setName(variable);
				vars.insert(std::move(varr));
			}

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::Button("Delete"))
				{
					ImGui::CloseCurrentPopup();
					for (Plot* plt : *plotHandler)
						plt->removeSeries(var->getName());
					vars.erase(keyName);
				}
				ImGui::EndPopup();
			}

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
		ImGui::EndTable();
	}
}

void Gui::drawPlotsTree()
{
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	std::string newName = "";

	if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_Reorderable))
	{
		if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
			plotHandler->addPlot("new plot");

		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Button("Add plot"))
			{
				ImGui::CloseCurrentPopup();
				plotHandler->addPlot("new plot");
			}
			ImGui::EndPopup();
		}

		for (Plot* plt : *plotHandler)
		{
			const char* plotTypes[3] = {"curve", "bar", "table"};
			int32_t typeCombo = (int32_t)plt->getType();

			newName = plt->getName();

			if (ImGui::BeginTabItem(plt->getName().c_str()))
			{
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::Button("Delete plot"))
					{
						ImGui::CloseCurrentPopup();
						plotHandler->removePlot(plt->getName());
					}
					ImGui::EndPopup();
				}

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
				if (ImGui::BeginListBox("##", ImVec2(-1, 80)))
				{
					for (auto& [name, ser] : plt->getSeriesMap())
					{
						ImGui::Selectable(name.c_str());
						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::Button("Delete var"))
							{
								ImGui::CloseCurrentPopup();
								plt->removeSeries(name);
							}
							ImGui::EndPopup();
						}
					}
					ImGui::EndListBox();
				}
				ImGui::PopID();

				ImGui::TreePop();
			}

			if (typeCombo != (int32_t)plt->getType())
				plt->setType(static_cast<Plot::type_E>(typeCombo));

			if (ImGui::IsKeyPressed(ImGuiKey_Enter) && newName != plt->getName())
				plotHandler->renamePlot(plt->getName(), newName);
		}
		ImGui::EndTabBar();
	}
}

void Gui::drawAcqusitionSettingsWindow()
{
	ImGui::Begin("Acqusition Settings", &showAcqusitionSettingsWindow, 0);
	ImGui::Text("Please pick *.elf file");
	ImGui::InputText("##", &projectElfFile, 0, NULL, NULL);
	ImGui::SameLine();
	if (ImGui::SmallButton("..."))
	{
		nfdchar_t* outPath;
		nfdfilteritem_t filterItem[1] = {{"Executable files", "elf"}};
		nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
		if (result == NFD_OKAY)
		{
			std::cout << outPath << std::endl;
			projectElfFile = std::string(outPath);
			std::replace(projectElfFile.begin(), projectElfFile.end(), '\\', '/');
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
	}

	ImGui::End();
}

void Gui::drawPlot(Plot* plot, ScrollingBuffer<float>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap)
{
	if (!plot->getVisibility())
		return;

	ImVec2 plotSize = ImVec2(-1, (ImGui::GetWindowSize().y - 50) / plotHandler->getVisiblePlotsCount());

	if (plot->getType() == Plot::type_E::CURVE)
	{
		if (ImPlot::BeginPlot(plot->getName().c_str(), plotSize, ImPlotFlags_NoChild))
		{
			if (plotHandler->getViewerState())
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
				serPtr->buffer->copyData();
			uint32_t offset = time.getOffset();
			uint32_t size = time.getSize();
			mtx->unlock();

			for (auto& [key, serPtr] : seriesMap)
			{
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

std::string Gui::intToHexString(uint32_t var)
{
	std::stringstream ss;
	ss << std::hex << var;
	return ss.str();
}