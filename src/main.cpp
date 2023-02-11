
#include <stdio.h>

#include <mutex>

#include "Gui.hpp"
#include "PlotHandler.hpp"

std::mutex m;

int main(int ac, char** av)
{
	PlotHandler plotHandler(&m);
	Gui gui(&plotHandler, &m);

	return 0;
}
