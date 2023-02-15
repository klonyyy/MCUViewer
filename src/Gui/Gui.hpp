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

	Gui(PlotHandler* plotHandler);
	~Gui();

   private:
	state viewerState = state::STOP;
	const std::map<state, std::string> viewerStateMap{{state::RUN, "RUNNING"}, {state::STOP, "STOPPED"}};
	std::vector<Variable> vars;
	std::thread threadHandle;
	PlotHandler* plotHandler;
	std::mutex mtx;

	bool done = false;
	void mainThread();
	void drawMenu();
	void drawStartButton();
	void drawAddVariableButton();
	void drawVarTable();
	void drawPlotsTree();
	std::string intToHexString(uint32_t i);
};

#endif