
#pragma once
#include "glfw3.h"
#include <functional>

class OpenGLRenderer
{
   public:
	void init(GLFWwindow* window);
	void step(std::function<void()> guiFunction, bool shouldIncreaseFramerate);
	void deinit();

   private:
	GLFWwindow* window = nullptr;
};
