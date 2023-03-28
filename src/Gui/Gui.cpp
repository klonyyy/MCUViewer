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

Gui::Gui(PlotHandler* plotHandler, ConfigHandler* configHandler, bool& done) : plotHandler(plotHandler), configHandler(configHandler), done(done)
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
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
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

		for (uint32_t k = 0; k < plotHandler->getPlotsCount(); k++)
		{
			Plot* plt = plotHandler->getPlot(k);
			drawPlot(plt, plt->getTimeSeries(), plt->getSeriesMap());
		}

		drawMenu();
		ImGui::End();

		ImGui::Begin("VarViewer", &p_open, 0);
		drawStartButton();
		ImGui::SameLine();
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

	if (ImGui::Button(viewerStateMap.at(viewerState).c_str(), ImVec2(200, 50)))
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
	if (ImGui::Button("Add variable"))
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
	if (ImGui::Button("Update Variable addresses"))
		elfReader->updateVariableMap(vars);
}

void Gui::drawVarTable()
{
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

	ImVec2 outer_size = ImVec2(0.0f, 300);
	if (ImGui::BeginTable("table_scrolly", 3, flags, outer_size))
	{
		ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None);
		ImGui::TableHeadersRow();

		uint32_t row = 0;
		for (auto& [keyName, var] : vars)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(row++);

			ImGui::ColorEdit4("##Color", &var->getColor().r, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();

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
					for (uint32_t i = 0; i < plotHandler->getPlotsCount(); i++)
					{
						if (plotHandler->getPlot(i)->removeVariable(var->getAddress()))
						{
							std::cout << "deleting " << var->getName() << " for plot " << (int)i << "succeded" << std::endl;
						}
						else
						{
							std::cout << "deleting " << var->getName() << " for plot " << (int)i << "FAILED" << std::endl;
						}
					}

					vars.erase(keyName);
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload("MY_DND", &var->getName(), sizeof(var->getName()));
				ImPlot::ItemIcon(0xaabbcc);
				ImGui::SameLine();
				ImGui::TextUnformatted(var->getName().c_str());
				ImGui::EndDragDropSource();
			}
			ImGui::PopID();
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
	if (ImGui::TreeNode("Plots"))
	{
		for (uint32_t i = 0; i < plotHandler->getPlotsCount(); i++)
		{
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);

			if (ImGui::TreeNode((void*)(intptr_t)i, (plotHandler->getPlot(i)->getName()).c_str(), i))
			{
				ImGui::Text("blah blah");
				ImGui::SameLine();
				if (ImGui::SmallButton("button"))
				{
				}
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
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

void Gui::drawPlot(Plot* plot, ScrollingBuffer<float>& time, std::map<uint32_t, std::shared_ptr<Plot::Series>>& seriesMap)
{
	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(-1, 300), ImPlotFlags_NoChild))
	{
		ImPlot::SetupAxes("time[s]", NULL, 0, 0);
		ImPlot::SetupAxisLimits(ImAxis_X1, -1, 10, ImPlotCond_Once);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -0.1, 0.1, ImPlotCond_Once);
		ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
		ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);

		if (ImPlot::BeginDragDropTargetPlot())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
				plot->addSeries(*vars[*(std::string*)payload->Data]);
			ImPlot::EndDragDropTarget();
		}

		for (auto& ser : seriesMap)
		{
			ImPlot::SetNextLineStyle(ImVec4(ser.second->color->r, ser.second->color->g, ser.second->color->b, 1.0f));
			ImPlot::PlotLine(ser.second->seriesName->c_str(), time.getFirstElement(), ser.second->buffer->getFirstElement(), ser.second->buffer->getSize(), 0, ser.second->buffer->getOffset(), sizeof(float));
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
		}

		ImPlot::EndPlot();
	}
}

std::string Gui::intToHexString(uint32_t var)
{
	std::stringstream ss;
	ss << std::hex << var;
	return ss.str();
}