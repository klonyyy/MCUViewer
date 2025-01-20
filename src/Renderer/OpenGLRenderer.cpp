
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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
}

void OpenGLRenderer::step(std::function<void()> guiFunction, bool shouldIncreaseFramerate)
{
	ImGui_ImplOpenGL3_NewFrame();

	if (glfwGetWindowAttrib(window, GLFW_FOCUSED) || shouldIncreaseFramerate)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(4);

	guiFunction();

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
