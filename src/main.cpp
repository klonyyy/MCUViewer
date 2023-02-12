
#include <stdio.h>

#include "Gui.hpp"
#include "PlotHandler.hpp"

int main(int ac, char** av)
{
	PlotHandler plotHandler;
	Gui gui(&plotHandler);

	return 0;
}
