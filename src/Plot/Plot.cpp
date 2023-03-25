#include "Plot.hpp"

#include <unistd.h>

#include <iostream>

#include "implot.h"

Plot::Plot(std::string name) : name(name)
{
}
Plot::~Plot()
{
}

std::string Plot::getName() const
{
	return name;
}

bool Plot::addSeries(std::string* name, uint32_t address, Variable::Color& color)
{
	seriesPtr[address] = std::make_shared<Series>();
	seriesPtr[address]->buffer = std::make_unique<ScrollingBuffer<float>>();
	seriesPtr[address]->seriesName = name;
	seriesPtr[address]->color = &color;
	return true;
}

bool Plot::addSeries(Variable& var)
{
	uint32_t address = var.getAddress();
	seriesPtr[address] = std::make_shared<Series>();
	seriesPtr[address]->buffer = std::make_unique<ScrollingBuffer<float>>();
	seriesPtr[address]->seriesName = &var.getName();
	seriesPtr[address]->type = var.getType();
	seriesPtr[address]->color = &var.getColor();

	return true;
}

std::shared_ptr<Plot::Series> Plot::getSeries(uint32_t address)
{
	return seriesPtr[address];
}

std::map<uint32_t, std::shared_ptr<Plot::Series>>& Plot::getSeriesMap()
{
	return seriesPtr;
}

ScrollingBuffer<float>& Plot::getTimeSeries()
{
	return time;
}

bool Plot::removeVariable(uint32_t address)
{
	seriesPtr[address]->buffer.reset();
	seriesPtr[address].reset();
	seriesPtr.erase(address);
	return true;
}
bool Plot::removeAllVariables()
{
	for (auto& addr : seriesPtr)
	{
		seriesPtr[addr.first]->buffer.reset();
		seriesPtr[addr.first].reset();
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

void Plot::erase()
{
	time.erase();

	for (auto& ser : seriesPtr)
		ser.second->buffer->erase();
}
