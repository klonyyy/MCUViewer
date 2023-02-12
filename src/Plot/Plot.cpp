#include "Plot.hpp"

#include <unistd.h>

#include <iostream>

#include "implot.h"

Plot::Plot(std::string name) : name(name)
{
}
Plot::~Plot()
{
	removeAllVariables();
}
bool Plot::addSeries(std::string name, uint32_t address)
{
	seriesPtr[address] = new Series();
	seriesPtr[address]->buffer = new ScrollingBuffer<float>();
	seriesPtr[address]->seriesName = name;

	return true;
}

bool Plot::addSeries(Variable var)
{
	uint32_t address = var.getAddress();
	seriesPtr[address] = new Series();
	seriesPtr[address]->buffer = new ScrollingBuffer<float>();
	seriesPtr[address]->seriesName = var.getName();
	seriesPtr[address]->type = var.getType();

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

std::vector<Variable::type> Plot::getVariableTypes()
{
	std::vector<Variable::type> types;

	for (auto& entry : seriesPtr)
		types.push_back(entry.second->type);

	return types;
}

bool Plot::addPoint(float t, uint32_t address, float value)
{
	mtx.lock();
	seriesPtr[address]->buffer->addPoint(value);
	mtx.unlock();
	return true;
}

bool Plot::addTimePoint(float t)
{
	time.addPoint(t);
	return true;
}

void Plot::draw()
{
	if (ImPlot::BeginPlot(name.c_str(), ImVec2(-1, 300), ImPlotFlags_NoChild))
	{
		ImPlot::SetupAxes("time[s]", NULL, 0, 0);
		ImPlot::SetupAxisLimits(ImAxis_X1, -1, 10, ImPlotCond_Once);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -0.1, 0.1, ImPlotCond_Once);
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

void Plot::erase()
{
	time.erase();

	for (auto& ser : seriesPtr)
		ser.second->buffer->erase();
}
