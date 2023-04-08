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

void Plot::setName(std::string newName)
{
	name = newName;
}

std::string Plot::getName() const
{
	return name;
}

std::string& Plot::getNameVar()
{
	return name;
}

bool Plot::addSeries(std::string* name, uint32_t address, Variable::Color& color)
{
	seriesMap[address] = std::make_shared<Series>();
	seriesMap[address]->buffer = std::make_unique<ScrollingBuffer<float>>();
	seriesMap[address]->seriesName = name;
	seriesMap[address]->color = &color;
	return true;
}

bool Plot::addSeries(Variable& var)
{
	uint32_t address = var.getAddress();
	seriesMap[address] = std::make_shared<Series>();
	seriesMap[address]->buffer = std::make_unique<ScrollingBuffer<float>>();
	seriesMap[address]->seriesName = &var.getName();
	seriesMap[address]->type = var.getType();
	seriesMap[address]->color = &var.getColor();

	return true;
}

std::shared_ptr<Plot::Series> Plot::getSeries(uint32_t address)
{
	return seriesMap[address];
}

std::map<uint32_t, std::shared_ptr<Plot::Series>>& Plot::getSeriesMap()
{
	return seriesMap;
}

ScrollingBuffer<float>& Plot::getTimeSeries()
{
	return time;
}

bool Plot::removeVariable(uint32_t address)
{
	if (seriesMap.find(address) == seriesMap.end())
		return false;

	seriesMap[address]->buffer.reset();
	seriesMap[address].reset();
	seriesMap.erase(address);
	return true;
}
bool Plot::removeAllVariables()
{
	for (auto& addr : seriesMap)
	{
		seriesMap[addr.first]->buffer.reset();
		seriesMap[addr.first].reset();
	}
	seriesMap.clear();
	return true;
}

std::vector<uint32_t> Plot::getVariableAddesses()
{
	std::vector<uint32_t> addresses;

	for (auto& addr : seriesMap)
		addresses.push_back(addr.first);

	return addresses;
}

std::vector<Variable::type> Plot::getVariableTypes()
{
	std::vector<Variable::type> types;

	for (auto& entry : seriesMap)
		types.push_back(entry.second->type);

	return types;
}

bool Plot::addPoint(uint32_t address, float value)
{
	seriesMap[address]->buffer->addPoint(value);
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

	for (auto& ser : seriesMap)
		ser.second->buffer->erase();
}

void Plot::setVisibility(bool state)
{
	visibility = state;
}
bool Plot::getVisibility()
{
	return visibility;
}
bool& Plot::getVisibilityVar()
{
	return visibility;
}

void Plot::setType(type_E newType)
{
	type = newType;
}
Plot::type_E Plot::getType()
{
	return type;
}
