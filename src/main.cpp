
#include <stdio.h>

#include "ConfigHandler.hpp"
#include "Gui.hpp"
#include "PlotHandler.hpp"

bool done = false;

int main(int ac, char** av)
{
	PlotHandler plotHandler(done);
	ConfigHandler configHandler("", &plotHandler);
	Gui gui(&plotHandler, &configHandler, done);

	while (!done)
	{
	}
	std::cout << "CLOSING" << std::endl;

	return 0;
}
