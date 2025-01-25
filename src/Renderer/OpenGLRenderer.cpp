
#include "OpenGLRenderer.hpp"

#include <string>

#include "GuiHelper.hpp"
#include "OpenGLRenderer.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void OpenGLRenderer::init(GLFWwindow* window)
{
	this->window = window;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	glfwShowWindow(window);
}

void OpenGLRenderer::step(std::function<void()> guiFunction, bool shouldIncreaseFramerate)
{
	if (glfwGetWindowAttrib(window, GLFW_FOCUSED) || shouldIncreaseFramerate)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(4);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

	guiFunction();

	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(window);
}

void OpenGLRenderer::deinit()
{
	ImGui_ImplOpenGL3_Shutdown();
}
