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
bool Plot::addSeries(std::string name, uint32_t address)
{
	seriesPtr[address] = new Series();
	seriesPtr[address]->buffer = new ScrollingBuffer<float>();
	seriesPtr[address]->seriesName = name;

	std::cout << "Active Series:" << std::endl;
	for (auto& series : seriesPtr)
		std::cout << " - " << series.second->seriesName << std::endl;

	return true;
}
bool Plot::removeVariable(uint32_t address)
{
	delete seriesPtr[address]->buffer;
	delete seriesPtr[address];
	seriesPtr.erase(address);
	return true;
}
bool Plot::removeAllVariables()
{
	for (auto& addr : seriesPtr)
	{
		delete seriesPtr[addr.first]->buffer;
		delete seriesPtr[addr.first];
	}
	seriesPtr.clear();
	return true;
}

std::vector<uint32_t> Plot::getVariableAddesses()
{
	std::vector<uint32_t> addresses;

	for (auto& addr : seriesPtr)
		addresses.push_back(addr.first);

	return addresses;
}

bool Plot::addPoint(float t, uint32_t address, float value)
{
	seriesPtr[address]->buffer->addPoint(value);
	time.addPoint(t);

	return true;
}

void Plot::draw()
{
	if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 300), ImPlotFlags_NoChild))
	{
		ImPlot::SetupAxes("time[s]", NULL, 0, 0);
		ImPlot::SetupAxisLimits(ImAxis_X1, 0, 10, ImPlotCond_Once);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 0.1, ImPlotCond_Once);
		ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
		ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);

		for (auto& ser : seriesPtr)
		{
			ImPlot::PlotLine(ser.second->seriesName.c_str(), time.getFirstElement(), ser.second->buffer->getFirstElement(), ser.second->buffer->getSize(), 0, ser.second->buffer->getOffset(), sizeof(float));
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
		}

		ImPlot::EndPlot();
	}
}
