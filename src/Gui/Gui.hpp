#ifndef _GUI_HPP
#define _GUI_HPP

#include <mutex>
#include <thread>

#include "PlotHandler.hpp"
#include "imgui.h"

class Gui
{
   public:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};

	Gui(PlotHandler* plotHandler, std::mutex* mtx);
	~Gui();

   private:
	state viewerState = state::STOP;
	const std::map<state, std::string> viewerStateMap{{state::RUN, "RUNNING"}, {state::STOP, "STOPPED"}};
	std::thread threadHandle;
	std::vector<uint32_t> addresses;
	PlotHandler* plotHandler;
	std::mutex* mtx;

	bool done = false;
	void mainThread();
	void drawMenu();
	void drawStartButton();
};

#endif