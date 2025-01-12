
#include "OpenGLRenderer.hpp"

#include <string>

#include "GuiHelper.hpp"
#include "OpenGLRenderer.hpp"
#include "glfw3.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

void OpenGLRenderer::init(GLFWwindow* window)
{
	this->window = window;
	ImGui_ImplOpenGL3_Init("#version 130");
}

void OpenGLRenderer::stepEnter()
{
	ImGui_ImplOpenGL3_NewFrame();
}

void OpenGLRenderer::stepExit()
{
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(window);
}

void OpenGLRenderer::deinit()
{
	ImGui_ImplOpenGL3_Shutdown();
}
