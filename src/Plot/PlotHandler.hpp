#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include "Plot.hpp"

class PlotHandler
{
   public:
	PlotHandler();
	~PlotHandler();

	int addPlot();
	bool removePlot(int id);

   private:
	static Plot* plotPtrList;
};

#endif