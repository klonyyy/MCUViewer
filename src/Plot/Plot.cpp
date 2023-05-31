#include "Plot.hpp"

#include <unistd.h>

#include <bitset>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

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
	seriesMap[name]->buffer = std::make_unique<ScrollingBuffer<double>>();
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

ScrollingBuffer<double>& Plot::getTimeSeries()
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

bool Plot::addPoint(const std::string& varName, double value)
{
	seriesMap[varName]->var->setValue(value);
	seriesMap[varName]->buffer->addPoint(value);
	return true;
}

bool Plot::addTimePoint(double t)
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

void Plot::setType(type_E newType)
{
	type = newType;
}
Plot::type_E Plot::getType() const
{
	return type;
}

Plot::displayFormat Plot::getSeriesDisplayFormat(const std::string& name) const
{
	return seriesMap.at(name)->format;
}
void Plot::setSeriesDisplayFormat(const std::string& name, displayFormat format)
{
	seriesMap.at(name)->format = format;
}

std::string Plot::getSeriesValueString(const std::string& name, double value)
{
	Variable::type type = seriesMap.at(name)->var->getType();

	if (type == Variable::type::F32)
		return std::to_string(value);

	switch (seriesMap.at(name)->format)
	{
		case displayFormat::DEC:
			return std::to_string(value);
		case displayFormat::HEX:
		{
			std::stringstream ss;
			ss << std::hex << static_cast<int64_t>(value);
			return std::string("0x") + ss.str();
		}
		case displayFormat::BIN:
		{
			switch (type)
			{
				case Variable::type::I8:
				case Variable::type::U8:
				{
					std::bitset<8> binaryValue(value);
					return std::string("0b") + binaryValue.to_string();
				}
				case Variable::type::I16:
				case Variable::type::U16:
				{
					std::bitset<16> binaryValue(value);
					return std::string("0b") + binaryValue.to_string();
				}
				case Variable::type::I32:
				case Variable::type::U32:
				{
					std::bitset<32> binaryValue(value);
					return std::string("0b") + binaryValue.to_string();
				}
				default:
					break;
			}
			break;
		}
		default:
			return std::string("");
	}
	return std::string("");
}

bool Plot::getMarkerStateX0()
{
	return mx0.state;
}

void Plot::setMarkerStateX0(bool state)
{
	mx0.state = state;
}

double Plot::getMarkerValueX0()
{
	return mx0.value;
}

void Plot::setMarkerValueX0(double value)
{
	mx0.value = value;
}

bool Plot::getMarkerStateX1()
{
	return mx1.state;
}

void Plot::setMarkerStateX1(bool state)
{
	mx1.state = state;
}

double Plot::getMarkerValueX1()
{
	return mx1.value;
}
void Plot::setMarkerValueX1(double value)
{
	mx1.value = value;
}
