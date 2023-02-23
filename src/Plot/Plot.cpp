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

std::string Plot::getName() const
{
	return name;
}

bool Plot::addSeries(std::string* name, uint32_t address)
{
	seriesPtr[address] = new Series();
	seriesPtr[address]->buffer = new ScrollingBuffer<float>();
	seriesPtr[address]->seriesName = name;

	return true;
}

bool Plot::addSeries(Variable& var)
{
	uint32_t address = var.getAddress();
	seriesPtr[address] = new Series();
	seriesPtr[address]->buffer = new ScrollingBuffer<float>();
	seriesPtr[address]->seriesName = &var.getName();
	seriesPtr[address]->type = var.getType();

	return true;
}

Plot::Series& Plot::getSeries(uint32_t address)
{
	return *seriesPtr[address];
}

std::map<uint32_t, Plot::Series*>& Plot::getSeriesMap()
{
	return seriesPtr;
}

ScrollingBuffer<float>& Plot::getTimeSeries()
{
	return time;
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

void Plot::erase()
{
	time.erase();

	for (auto& ser : seriesPtr)
		ser.second->buffer->erase();
}
