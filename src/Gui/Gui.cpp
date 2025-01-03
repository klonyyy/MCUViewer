#include "Gui.hpp"

#include <imgui.h>
#include <unistd.h>

#include <future>
#include <set>
#include <sstream>
#include <string>
#include <utility>

#include "PlotHandler.hpp"
#include "Statistics.hpp"
#include "StlinkDebugProbe.hpp"
#ifndef __APPLE__
#include "glfw3.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#else
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include <stdio.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "Statistics.hpp"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

Gui::Gui(PlotHandler* plotHandler, VariableHandler* variableHandler, ConfigHandler* configHandler, PlotGroupHandler* plotGroupHandler, IFileHandler* fileHandler, PlotHandler* tracePlotHandler, ViewerDataHandler* viewerDataHandler, TraceDataHandler* traceDataHandler, std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger, std::string& projectPath) : plotHandler(plotHandler), variableHandler(variableHandler), configHandler(configHandler), plotGroupHandler(plotGroupHandler), fileHandler(fileHandler), tracePlotHandler(tracePlotHandler), viewerDataHandler(viewerDataHandler), traceDataHandler(traceDataHandler), done(done), mtx(mtx), logger(logger)
{
	#ifndef __APPLE__
	threadHandle = std::thread(&Gui::mainThread, this, projectPath);
	#endif
	plotEditWindow = std::make_shared<PlotEditWindow>(plotHandler, plotGroupHandler, variableHandler);
	plotsTree = std::make_shared<PlotsTree>(viewerDataHandler, plotHandler, plotGroupHandler, variableHandler, plotEditWindow, fileHandler, logger);
	variableTable = std::make_shared<VariableTableWindow>(viewerDataHandler, plotHandler, variableHandler, &projectElfPath, &projectConfigPath, logger);

	variableHandler->renameCallback = [&](std::string oldName, std::string newName)
	{
		for (std::shared_ptr<Plot> plt : *this->plotHandler)
			plt->renameSeries(oldName, newName);
	};

	#ifdef __APPLE__
	mainThread(projectPath);
	#endif
}

Gui::~Gui()
{	
	#ifndef __APPLE__
	if (threadHandle.joinable())
		threadHandle.join();
	#else
	;
	#endif
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static float getContentScale(GLFWwindow* window)
{
	float xscale;
	float yscale;
	float scaleFactor;
	#ifdef __APPLE__
	scaleFactor = 4.0;
	#else
	scaleFactor = 2.0;
	#endif
	glfwGetWindowContentScale(window, &xscale, &yscale);
	return (xscale + yscale) / scaleFactor;
}

void Gui::mainThread(std::string externalPath)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return;

	#ifdef __APPLE__
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	#endif
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

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.Fonts->AddFontDefault(&cfg);
	io.FontGlobalScale = 1.0f;

	ImGui::StyleColorsDark();
	ImPlot::StyleColorsDark();

	#ifdef __APPLE__
	id <MTLDevice> device = MTLCreateSystemDefaultDevice();
	id <MTLCommandQueue> commandQueue = [device newCommandQueue];
	#endif

	ImGui::GetStyle().ScaleAllSizes(GuiHelper::contentScale);
	ImGui::GetStyle().Colors[ImGuiCol_PopupBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

	#ifndef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	#else
	ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplMetal_Init(device);
	#endif

	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

	#ifdef __APPLE__
    NSWindow *nswin = glfwGetCocoaWindow(window);
    CAMetalLayer *layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    nswin.contentView.layer = layer;
    nswin.contentView.wantsLayer = YES;

    MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor new];
	#endif

	fileHandler->init();

	#ifndef __APPLE__
	jlinkProbe = std::make_shared<JlinkDebugProbe>(logger);
	#endif

	stlinkProbe = std::make_shared<StlinkDebugProbe>(logger);
	debugProbeDevice = stlinkProbe;
	viewerDataHandler->setDebugProbe(debugProbeDevice);

	#ifndef __APPLE__
	jlinkTraceProbe = std::make_shared<JlinkTraceProbe>(logger);
	#endif

	stlinkTraceProbe = std::make_shared<StlinkTraceProbe>(logger);
	traceProbeDevice = stlinkTraceProbe;
	traceDataHandler->setDebugProbe(traceProbeDevice);

	float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

	if (!externalPath.empty())
		openProject(externalPath);

	while (!done)
	{	
		#ifdef __APPLE__
		@autoreleasepool
        {
		#else
			if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
			{
				glfwWaitEvents();
				continue;
			}

			if (glfwGetWindowAttrib(window, GLFW_FOCUSED) || (traceDataHandler->getState() == DataHandlerBase::State::RUN) || (viewerDataHandler->getState() == DataHandlerBase::State::RUN))
				glfwSwapInterval(1);
			else
				glfwSwapInterval(4);
		#endif
			glfwSetWindowTitle(window, (std::string("MCUViewer - ") + projectConfigPath).c_str());
			glfwPollEvents();
			
			#ifdef __APPLE__
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			layer.drawableSize = CGSizeMake(width, height);
			id<CAMetalDrawable> drawable = [layer nextDrawable];

			id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
			renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3]);
			renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
			renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
			renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
			id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
			[renderEncoder pushDebugGroup:@"ImGui demo"];

			ImGui_ImplMetal_NewFrame(renderPassDescriptor);
			#else
			ImGui_ImplOpenGL3_NewFrame();
			#endif

			ImGui_ImplGlfw_NewFrame();

			ImGui::NewFrame();
			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

			if (showDemoWindow)
				ImPlot::ShowDemoWindow();

			if (glfwWindowShouldClose(window))
			{
				viewerDataHandler->setState(DataHandlerBase::State::STOP);
				traceDataHandler->setState(DataHandlerBase::State::STOP);

				if (configHandler->isSavingRequired(projectElfPath))
					askShouldSaveOnExit(glfwWindowShouldClose(window));
				else
					done = true;
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
				drawStartButton(traceDataHandler);
				drawSettingsSwo();
				drawIndicatorsSwo();
				drawPlotsTreeSwo();
			}
			ImGui::End();

			if (ImGui::Begin("Var Viewer"))
			{
				activeView = ActiveViewType::VarViewer;
				drawAcqusitionSettingsWindow(activeView);
				drawStartButton(viewerDataHandler);
				variableTable->draw();
				plotsTree->draw();
				plotEditWindow->draw();
				ImGui::SetNextWindowClass(&window_class);
				if (ImGui::Begin("Plots"))
					drawPlots();
				ImGui::End();
			}
			ImGui::End();

			popup.handle();

			// Rendering
			ImGui::Render();
			#ifndef __APPLE__
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			#else
			ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);
			#endif

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}
			#ifndef __APPLE__
			glfwSwapBuffers(window);
			#else
			[renderEncoder popDebugGroup];
			[renderEncoder endEncoding];

			[commandBuffer presentDrawable:drawable];
			[commandBuffer commit];
		}
		#endif
	}

	logger->info("Exiting GUI main thread");
	
	#ifndef __APPLE__
	ImGui_ImplOpenGL3_Shutdown();
	#else
	ImGui_ImplMetal_Shutdown();
	#endif
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

	bool active = !(viewerDataHandler->getState() == DataHandlerBase::State::RUN || traceDataHandler->getState() == DataHandlerBase::State::RUN);

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
		GuiHelper::drawDescriptionWithNumber("sampling: ", viewerDataHandler->getAverageSamplingFrequency(), " Hz", 2);
	}

	ImGui::EndMainMenuBar();
	askShouldSaveOnExit(shouldSaveOnClose);
	askShouldSaveOnNew(shouldSaveOnNew);
}

void Gui::drawStartButton(DataHandlerBase* activeDataHandler)
{
	bool shouldDisableButton = (!devicesList.empty() && devicesList.front() == noDevices);
	ImGui::BeginDisabled(shouldDisableButton);

	DataHandlerBase::State state = activeDataHandler->getState();

	if (state == DataHandlerBase::State::RUN)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, GuiHelper::green);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiHelper::greenLight);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, GuiHelper::greenLightDim);
	}
	else if (state == DataHandlerBase::State::STOP)
	{
		if (activeDataHandler->getLastReaderError() != "")
		{
			ImGui::PushStyleColor(ImGuiCol_Button, GuiHelper::red);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiHelper::redLight);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, GuiHelper::redLightDim);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, GuiHelper::orange);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GuiHelper::orangeLight);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, GuiHelper::orangeLightDim);
		}
	}

	if ((ImGui::Button((viewerStateMap.at(state) + " " + activeDataHandler->getLastReaderError()).c_str(), ImVec2(-1, 50 * GuiHelper::contentScale)) ||
		 (ImGui::IsKeyPressed(ImGuiKey_Space, false) && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup))) &&
		!shouldDisableButton)
	{
		if (state == DataHandlerBase::State::STOP)
		{
			logger->info("Start clicked!");
			plotHandler->eraseAllPlotData();
			tracePlotHandler->eraseAllPlotData();
			activeDataHandler->setState(DataHandlerBase::State::RUN);
		}
		else
		{
			logger->info("Stop clicked!");
			activeDataHandler->setState(DataHandlerBase::State::STOP);
		}
	}

	ImGui::PopStyleColor(3);
	ImGui::EndDisabled();
}

void Gui::drawAcqusitionSettingsWindow(ActiveViewType type)
{
	if (showAcqusitionSettingsWindow)
		ImGui::OpenPopup("Acqusition Settings");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(950 * GuiHelper::contentScale, 600 * GuiHelper::contentScale));
	if (ImGui::BeginPopupModal("Acqusition Settings", &showAcqusitionSettingsWindow, 0))
	{
		if (type == ActiveViewType::VarViewer)
			acqusitionSettingsViewer();
		else if (type == ActiveViewType::TraceViewer)
			acqusitionSettingsTrace();

		acqusitionErrorPopup.handle();

		const float buttonHeight = 25.0f * GuiHelper::contentScale;
		ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));

		if (ImGui::Button("Done", ImVec2(-1, buttonHeight)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
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
		if (ImGui::Button("Done", ImVec2(-1, buttonHeight)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			showPreferencesWindow = false;
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

	if (variableHandler->isEmpty() && projectElfPath.empty() && shouldOpenPopup)
		done = true;

	GuiHelper::showQuestionBox("Save?", "Do you want to save the current config?\n", onYes, onNo, onCancel);
}

void Gui::askShouldSaveOnNew(bool shouldOpenPopup)
{
	auto onNo = [&]()
	{
		variableHandler->clear();
		plotHandler->removeAllPlots();
		traceDataHandler->initPlots();
		plotGroupHandler->removeAllGroups();
		projectElfPath = "";
		projectConfigPath = "";
	};

	if (variableHandler->isEmpty() && projectElfPath.empty() && shouldOpenPopup)
		onNo();
	else if (shouldOpenPopup)
		ImGui::OpenPopup("SaveOnNew?");

	auto onYes = [&]()
	{
		if (!saveProject())
			saveProjectAs();
		onNo();
	};

	GuiHelper::showQuestionBox("SaveOnNew?", "Do you want to save the current config?\n", onYes, onNo, []() {});
}

bool Gui::saveProject()
{
	if (!projectConfigPath.empty())
		return configHandler->saveConfigFile(projectElfPath, "");
	return false;
}

bool Gui::saveProjectAs()
{
	std::string path = fileHandler->saveFile(std::pair<std::string, std::string>("Project files", "cfg"));
	if (path != "")
	{
		projectConfigPath = path;
		configHandler->saveConfigFile(projectElfPath, projectConfigPath);
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
		variableHandler->clear();
		plotHandler->removeAllPlots();
		configHandler->readConfigFile(projectElfPath);

		logger->info("Project config path: {}", projectConfigPath);
		/* TODO refactor */
		devicesList.clear();
		if (viewerDataHandler->getProbeSettings().debugProbe == 1)
			#ifndef __APPLE__
			debugProbeDevice = jlinkProbe;
			#else
			;
			#endif
		else
			debugProbeDevice = stlinkProbe;

		viewerDataHandler->setDebugProbe(debugProbeDevice);

		if (traceDataHandler->getProbeSettings().debugProbe == 1)
			#ifndef __APPLE__
			traceProbeDevice = jlinkTraceProbe;
			#else
			;
			#endif
		else
			traceProbeDevice = stlinkTraceProbe;

		traceDataHandler->setDebugProbe(traceProbeDevice);

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

	if (plt.getSeries(name)->var->getType() == Variable::Type::F32)
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
