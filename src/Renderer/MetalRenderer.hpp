#pragma once
#include "glfw3.h"

class MetalRenderer
{
   public:
	void init(GLFWwindow* window);
	void stepEnter(bool shouldIncreaseFramerate);
	void stepExit();
	void deinit();

   private:
	GLFWwindow* window = nullptr;
};
