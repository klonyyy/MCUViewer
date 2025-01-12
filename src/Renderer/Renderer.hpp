#pragma once

#include <stdbool.h>

#include "glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "implot.h"

#ifdef APPLE
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
		this->windowName = windowName;

		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			return false;

		window = glfwCreateWindow(1500, 1000, windowName.c_str(), NULL, NULL);
		if (window == NULL)
			return false;

		glfwMakeContextCurrent(window);
		glfwMaximizeWindow(window);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		ImGui_ImplGlfw_InitForOpenGL(window, true);

		backend.init();

		return true;
	}

	void stepEnter(bool shouldIncreaseFramerate)
	{
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
		{
			glfwWaitEvents();
			return;
		}

		if (glfwGetWindowAttrib(window, GLFW_FOCUSED) || shouldIncreaseFramerate)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(4);

		glfwSetWindowTitle(window, windowName.c_str());
		glfwPollEvents();

		backend.stepEnter();

		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);
		;
	}

	void stepExit()
	{
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		backend.stepExit();
		glfwSwapBuffers(window);
	}

	void deinit()
	{
		backend.deinit();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

   private:
	std::string windowName;
};