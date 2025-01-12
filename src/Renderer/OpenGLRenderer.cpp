
#include "OpenGLRenderer.hpp"

#include <string>

#include "GuiHelper.hpp"
#include "OpenGLRenderer.hpp"
#include "glfw3.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

void OpenGLRenderer::init()
{
	ImGui_ImplOpenGL3_Init("#version 130");
}

void OpenGLRenderer::stepEnter()
{
	ImGui_ImplOpenGL3_NewFrame();
}

void OpenGLRenderer::stepExit()
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OpenGLRenderer::deinit()
{
	ImGui_ImplOpenGL3_Shutdown();
}
