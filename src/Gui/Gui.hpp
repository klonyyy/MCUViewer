#ifndef _GUI_HPP
#define _GUI_HPP

#include <thread>

class Gui
{
   public:
	Gui();
	~Gui();

   private:
	std::thread threadHandle;

	void mainThread();
	void drawMenu();
};

#endif