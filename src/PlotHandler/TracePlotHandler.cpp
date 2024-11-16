#include "TracePlotHandler.hpp"

#include <algorithm>
#include <memory>
#include <string>

void TracePlotHandler::initPlots()
{
	const uint32_t colors[] = {4294967040, 4294960666, 4294954035, 4294947661, 4294941030, 4294934656, 4294928025, 4294921651, 4294915020, 4294908646, 4294902015};

	for (uint32_t i = 0; i < channels; i++)
	{
		std::string name = std::string("CH" + std::to_string(i));
		plotsMap[name] = std::make_shared<Plot>(name);

		auto newVar = std::make_shared<Variable>(name);
		newVar->setColor(colors[i]);
		traceVars[name] = newVar;

		plotsMap[name]->addSeries(newVar.get());
		plotsMap[name]->setDomain(Plot::Domain::DIGITAL);
		plotsMap[name]->setAlias("CH" + std::to_string(i));
	}
}

TracePlotHandler::Settings TracePlotHandler::getSettings() const
{
	return settings;
}

void TracePlotHandler::setSettings(const Settings& settings)
{
	setMaxPoints(settings.maxPoints);
	this->settings = settings;
}

double TracePlotHandler::getDoubleValue(const Plot& plot, uint32_t value)
{
	if (plot.getDomain() == Plot::Domain::DIGITAL)
		return value == 0xaa ? 1.0 : 0.0;
	else if (plot.getDomain() == Plot::Domain::ANALOG)
	{
		switch (plot.getTraceVarType())
		{
			/*TODO: consider the bitcast solution, though the size of input and outpu differ */
			case Plot::TraceVarType::U8:
				return static_cast<double>(*reinterpret_cast<uint8_t*>(&value));
			case Plot::TraceVarType::I8:
				return static_cast<double>(*reinterpret_cast<int8_t*>(&value));
			case Plot::TraceVarType::U16:
				return static_cast<double>(*reinterpret_cast<uint16_t*>(&value));
			case Plot::TraceVarType::I16:
				return static_cast<double>(*reinterpret_cast<int16_t*>(&value));
			case Plot::TraceVarType::U32:
				return static_cast<double>(*reinterpret_cast<uint32_t*>(&value));
			case Plot::TraceVarType::I32:
				return static_cast<double>(*reinterpret_cast<int32_t*>(&value));
			case Plot::TraceVarType::F32:
				return static_cast<double>(*reinterpret_cast<float*>(&value));
			default:
				return static_cast<double>(*reinterpret_cast<uint32_t*>(&value));
		}
	}
	return 0.0;
}
