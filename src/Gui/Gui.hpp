#ifndef _GUI_HPP
#define _GUI_HPP

#include <thread>

#include "ScrollingBuffer.hpp"
#include "VarReader.hpp"
#include "imgui.h"

class Gui
{
   public:
	Gui();
	~Gui();

	void begin();

   private:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};
	state viewerState = state::STOP;
	VarReader* vals;
	std::thread threadHandle;
	std::thread dataHandle;

	ScrollingBuffer<float> time;
	ScrollingBuffer<float> sdata1;
	ScrollingBuffer<float> sdata2;

	std::chrono::time_point<std::chrono::steady_clock> start;

	float t = 0;
	bool done = false;

	void mainThread();
	void drawMenu();
	void dataHandler();
};

#endif