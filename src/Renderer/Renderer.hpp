#pragma once

#include <stdbool.h>

#include <functional>

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
		this->windowName = windowName;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();

		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			return false;

#ifdef __APPLE__
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#endif

		window = glfwCreateWindow(1500, 1000, windowName.c_str(), nullptr, nullptr);
		if (window == nullptr)
			return false;

#ifndef __APPLE__
		glfwMakeContextCurrent(window);
#endif

		glfwMaximizeWindow(window);
		backend.init(window);

		return true;
	}

	bool step(std::function<void()> guiFunction, bool shouldIncreaseFramerate)
	{
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
		{
			glfwWaitEvents();
			return false;
		}

		glfwSetWindowTitle(window, windowName.c_str());
		glfwPollEvents();

		backend.step(guiFunction, shouldIncreaseFramerate);
		return true;
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