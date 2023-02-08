#ifndef _PLOT_HPP
#define _PLOT_HPP

#include <thread>

class Plot
{
   public:
	Plot();
	~Plot();
	bool start();
	bool stop();
	void draw();

   private:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};

	state plotterState = state::STOP;
	std::thread threadHandle;
	void threadHandler();
};

#endif