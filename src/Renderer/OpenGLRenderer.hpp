
#pragma once
#include "glfw3.h"

class OpenGLRenderer
{
   public:
	void init(GLFWwindow* window);
	void stepEnter();
	void stepExit();
	void deinit();

   private:
	GLFWwindow* window = nullptr;
};
