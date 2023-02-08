#include "Plot.hpp"

#include <unistd.h>

#include <iostream>

#include "implot.h"

Plot::Plot()
{
}
Plot::~Plot()
{
}

bool Plot::start()
{
	if (plotterState == state::RUN)
		return false;

	std::cout << "Plot in run mode!" << std::endl;
	plotterState = state::RUN;
	threadHandle = std::thread(&Plot::threadHandler, this);
	return true;
}
bool Plot::stop()
{
	if (plotterState == state::STOP)
		return false;

	std::cout << "Plot in stop mode!" << std::endl;
	plotterState = state::STOP;
	threadHandle.join();
	return true;
}
void Plot::draw()
{
	// if (ImPlot::BeginPlot("##Plot", ImVec2(-1, 300), ImPlotFlags_NoFrame))
	// {
	// 	ImPlot::SetupAxes("time[s]", NULL, 0, 0);
	// 	ImPlot::SetupAxisLimits(ImAxis_X1, t - 10, t, ImPlotCond_Once);
	// 	ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1, ImPlotCond_Once);
	// 	ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
	// 	ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	ImPlot::PlotLine("Mouse Y", time.getFirstElement(), sdata2.getFirstElement(), sdata2.getSize(), 0, sdata2.getOffset(), sizeof(float));
	// 	ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	ImPlot::PlotLine("Mouse x", time.getFirstElement(), sdata1.getFirstElement(), sdata1.getSize(), 0, sdata1.getOffset(), sizeof(float));
	// 	ImPlot::EndPlot();
	// }
}
void Plot::threadHandler()
{
	while (plotterState == state::RUN)
	{
		usleep(100);
	}
}
