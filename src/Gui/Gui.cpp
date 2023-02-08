#include "Gui.hpp"

#include <SDL.h>
#include <SDL_opengl.h>
#include <unistd.h>

#include <mutex>

#include "VarReader.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "implot.h"
#include "iostream"

std::mutex mtx;

Gui::Gui()
{
	vals = new VarReader();
	threadHandle = std::thread(&Gui::mainThread, this);
	dataHandle = std::thread(&Gui::dataHandler, this);
}

Gui::~Gui()
{
	if (threadHandle.joinable())
		threadHandle.join();
	if (dataHandle.joinable())
		dataHandle.join();
}

void Gui::begin()
{
}

void Gui::mainThread()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
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

	window = SDL_CreateWindow("STMViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1);	// Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGuiStyle& style = ImGui::GetStyle();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	bool show_demo_window = false;
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

		ImGui::Begin("STMViewer", &p_open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar);

		static int clicked = 0;
		if (ImGui::Button("Button"))
			clicked++;
		if (clicked & 1)
		{
			if (viewerState == state::STOP)
			{
				viewerState = state::RUN;
				vals->start();
			}
			ImGui::SameLine();
			ImGui::Text("RUN");
		}
		else
		{
			vals->stop();
			viewerState = state::STOP;
			ImGui::SameLine();
			ImGui::Text("STOP");
		}

		drawMenu();

		static ImPlotAxisFlags flags = 0;
		mtx.lock();
		if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 300), ImPlotFlags_NoFrame))
		{
			ImPlot::SetupAxes("time[s]", NULL, flags, flags);
			ImPlot::SetupAxisLimits(ImAxis_X1, t - 10, t, ImPlotCond_Once);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1, ImPlotCond_Once);
			ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
			ImPlot::PlotLine("Mouse Y", time.getFirstElement(), sdata2.getFirstElement(), sdata2.getSize(), 0, sdata2.getOffset(), sizeof(float));
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
			ImPlot::PlotLine("Mouse x", time.getFirstElement(), sdata1.getFirstElement(), sdata1.getSize(), 0, sdata1.getOffset(), sizeof(float));
			ImPlot::EndPlot();
		}
		mtx.unlock();

		ImGui::End();

		// Rendering
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
	SDL_Quit();
}

void Gui::drawMenu()
{
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File"))
	{
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void Gui::dataHandler()
{
	while (!done)
	{
		start = std::chrono::steady_clock::now();
		std::this_thread::sleep_for(std::chrono::microseconds(1000));

		static float t = 0;

		mtx.lock();
		sdata1.addPoint(0.5f);
		sdata2.addPoint(vals->geta());
		time.addPoint(t);
		auto finish = std::chrono::steady_clock::now();
		double elapsed_seconds = std::chrono::duration_cast<
									 std::chrono::duration<double> >(finish - start)
									 .count();
		t += elapsed_seconds;
		mtx.unlock();
	}
}