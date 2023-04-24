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

void Plot::setName(const std::string& newName)
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

bool Plot::addSeries(Variable& var)
{
	std::string name = var.getName();
	seriesMap[name] = std::make_shared<Series>();
	seriesMap[name]->buffer = std::make_unique<ScrollingBuffer<float>>();
	seriesMap[name]->var = &var;
	return true;
}

std::shared_ptr<Plot::Series> Plot::getSeries(const std::string& name)
{
	return seriesMap[name];
}

std::map<std::string, std::shared_ptr<Plot::Series>>& Plot::getSeriesMap()
{
	return seriesMap;
}

ScrollingBuffer<float>& Plot::getTimeSeries()
{
	return time;
}

bool Plot::removeSeries(const std::string& name)
{
	if (seriesMap.find(name) == seriesMap.end())
		return false;

	seriesMap.erase(name);
	return true;
}
bool Plot::removeAllVariables()
{
	for (auto& [name, var] : seriesMap)
	{
		seriesMap[name]->buffer.reset();
		seriesMap[name].reset();
	}
	seriesMap.clear();
	return true;
}

std::vector<uint32_t> Plot::getVariableAddesses() const
{
	std::vector<uint32_t> addresses;
	for (auto& [name, ser] : seriesMap)
		addresses.push_back(ser->var->getAddress());

	return addresses;
}

std::vector<Variable::type> Plot::getVariableTypes() const
{
	std::vector<Variable::type> types;
	for (auto& [name, ser] : seriesMap)
		types.push_back(ser->var->getType());

	return types;
}

bool Plot::addPoint(const std::string& varName, const float value)
{
	seriesMap[varName]->buffer->addPoint(value);
	return true;
}

bool Plot::addTimePoint(const float t)
{
	time.addPoint(t);
	return true;
}

void Plot::erase()
{
	time.erase();

	for (auto& [name, ser] : seriesMap)
		ser->buffer->erase();
}

void Plot::setVisibility(bool state)
{
	visibility = state;
}
bool Plot::getVisibility() const
{
	return visibility;
}
bool& Plot::getVisibilityVar()
{
	return visibility;
}

void Plot::setType(const type_E newType)
{
	type = newType;
}
Plot::type_E Plot::getType() const
{
	return type;
}
