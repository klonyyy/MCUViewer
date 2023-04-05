
#include <stdio.h>

#include "ConfigHandler.hpp"
#include "Gui.hpp"
#include "PlotHandler.hpp"

bool done = false;
std::mutex mtx;

int main(int ac, char** av)
{
	PlotHandler plotHandler(done, &mtx);
	ConfigHandler configHandler("", &plotHandler);
	Gui gui(&plotHandler, &configHandler, done, &mtx);

	while (!done)
	{
	}
	std::cout << "CLOSING" << std::endl;

	return 0;
}
