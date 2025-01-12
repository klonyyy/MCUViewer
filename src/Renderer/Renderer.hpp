#pragma once

#include <stdbool.h>

#include "glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "implot.h"

#if defined(__APPLE__)
#include "MetalRenderer.hpp"
using RendererBackend = MetalRenderer;
#else
#include "OpenGLRenderer.hpp"
using RendererBackend = OpenGLRenderer;
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

class Renderer
{
   public:
	GLFWwindow* window = nullptr;
	RendererBackend backend;

	bool init(const std::string windowName)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();

		this->windowName = windowName;

		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			return false;
			
		#ifdef __APPLE__
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		#endif

		window = glfwCreateWindow(1500, 1000, windowName.c_str(), NULL, NULL);
		if (window == NULL)
			return false;

		#ifndef __APPLE__
			glfwMakeContextCurrent(window);
		#endif
		glfwMaximizeWindow(window);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		ImGui_ImplGlfw_InitForOpenGL(window, true);

		backend.init(window);

		return true;
	}

	bool stepEnter(bool shouldIncreaseFramerate)
	{
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
		{
			glfwWaitEvents();
			return false;
		}

		glfwSetWindowTitle(window, windowName.c_str());
		glfwPollEvents();

		backend.stepEnter(shouldIncreaseFramerate);

		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

		return true;
	}

	void stepExit()
	{
		ImGui::Render();
		backend.stepExit();
	}

	void deinit()
	{
		backend.deinit();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		
		if (window) 
		{
            glfwDestroyWindow(window);
            glfwTerminate();
        }
	}

   private:
	std::string windowName;
};