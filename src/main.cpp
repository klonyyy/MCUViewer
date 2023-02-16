
#include <stdio.h>

#include "ConfigHandler.hpp"
#include "Gui.hpp"
#include "PlotHandler.hpp"

PlotHandler plotHandler;

int main(int ac, char** av)
{
	ConfigHandler configHandler("/home/klonyyy/STMViewer/Project.cfg", &plotHandler);
	Gui gui(&plotHandler, &configHandler);

	std::cout << "CLOSING" << std::endl;

	return 0;
}
