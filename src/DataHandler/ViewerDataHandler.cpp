#include "ViewerDataHandler.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>

#include "JlinkDebugProbe.hpp"
#include "StlinkDebugProbe.hpp"

ViewerDataHandler::ViewerDataHandler(PlotGroupHandler* plotGroupHandler, VariableHandler* variableHandler, PlotHandler* plotHandler, TracePlotHandler* tracePlotHandler, std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger) : DataHandlerBase(plotGroupHandler, variableHandler, plotHandler, tracePlotHandler, done, mtx, logger)
{
	dataHandle = std::thread(&ViewerDataHandler::dataHandler, this);
	varReader = std::make_unique<MemoryReader>();
}
ViewerDataHandler::~ViewerDataHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
}

bool ViewerDataHandler::writeSeriesValue(Variable& var, double value)
{
	std::lock_guard<std::mutex> lock(*mtx);
	return varReader->setValue(var, value);
}

std::string ViewerDataHandler::getLastReaderError() const
{
	return varReader->getLastErrorMsg();
}

void ViewerDataHandler::setDebugProbe(std::shared_ptr<IDebugProbe> probe)
{
	varReader->changeDevice(probe);
}

IDebugProbe::DebugProbeSettings ViewerDataHandler::getProbeSettings() const
{
	return probeSettings;
}

void ViewerDataHandler::setProbeSettings(const IDebugProbe::DebugProbeSettings& settings)
{
	probeSettings = settings;
}

void ViewerDataHandler::dataHandler()
{
	std::chrono::time_point<std::chrono::steady_clock> start;
	uint32_t timer = 0;
	double lastT = 0.0;

	while (!done)
	{
		if (viewerState == state::RUN)
		{
			auto finish = std::chrono::steady_clock::now();
			double period = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();

			if (probeSettings.mode == IDebugProbe::Mode::HSS)
			{
				auto maybeEntry = varReader->readSingleEntry();

				if (!maybeEntry.has_value())
					continue;

				auto [timestamp, values] = maybeEntry.value();

				for (std::shared_ptr<Variable> var : *variableHandler)
				{
					if (values.contains(var->getAddress()))
					{
						var->setRawValueAndTransform(values.at(var->getAddress()));
						csvEntry[var->getName()] = var->getValue();
					}
				}

				for (auto plot : *plotHandler)
				{
					if (!plot->getVisibility())
						continue;

					std::lock_guard<std::mutex> lock(*mtx);
					/* thread-safe part */
					plot->updateSeries();
					plot->addTimePoint(timestamp);
				}

				if (plotHandler->getSettings().shouldLog)
					csvStreamer->writeLine(period, csvEntry);
				/* filter sampling frequency */
				averageSamplingPeriod = samplingPeriodFilter.filter((period - lastT));
				lastT = period;
				timer++;
			}

			else if (period > ((1.0 / plotHandler->getSettings().sampleFrequencyHz) * timer))
			{
				for (std::shared_ptr<Variable> var : *variableHandler)
				{
					bool result = false;
					uint32_t value = varReader->getValue(var->getAddress(), var->getSize(), result);

					if (result)
					{
						var->setRawValueAndTransform(value);
						csvEntry[var->getName()] = var->getValue();
					}
				}

				for (auto plot : *plotHandler)
				{
					if (!plot->getVisibility())
						continue;

					std::lock_guard<std::mutex> lock(*mtx);
					plot->updateSeries();
					plot->addTimePoint(period);
				}

				if (plotHandler->getSettings().shouldLog)
					csvStreamer->writeLine(period, csvEntry);
				/* filter sampling frequency */
				averageSamplingPeriod = samplingPeriodFilter.filter((period - lastT));
				lastT = period;
				timer++;
			}
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if (stateChangeOrdered)
		{
			if (viewerState == state::RUN)
			{
				auto addressSizeVector = createAddressSizeVector();

				prepareCSVFile();

				if (varReader->start(probeSettings, addressSizeVector, plotHandler->getSettings().sampleFrequencyHz))
				{
					timer = 0;
					lastT = 0.0;
					start = std::chrono::steady_clock::now();
				}
				else
					viewerState = state::STOP;
			}
			else
			{
				varReader->stop();
				if (plotHandler->getSettings().shouldLog)
					csvStreamer->finishLogging();
			}
			stateChangeOrdered = false;
		}
	}
}

std::vector<std::pair<uint32_t, uint8_t>> ViewerDataHandler::createAddressSizeVector()
{
	std::vector<std::pair<uint32_t, uint8_t>> addressSizeVector;

	for (auto& [name, plot] : *plotGroupHandler->getActiveGroup())
	{
		if (!plot->getVisibility())
			continue;

		for (auto& [name, ser] : plot->getSeriesMap())
		{
			// Get the address and size
			uint32_t address = ser->var->getAddress();
			uint8_t size = ser->var->getSize();
			std::pair<uint32_t, uint8_t> newElement = {address, size};

			// Check if the element already exists
			auto it = std::find_if(
				addressSizeVector.begin(),
				addressSizeVector.end(),
				[&newElement](const std::pair<uint32_t, uint8_t>& element)
				{
					return element == newElement;
				});

			if (it == addressSizeVector.end())
			{
				// Only add if the element is not already in the vector
				addressSizeVector.push_back(newElement);
			}
		}
	}

	return addressSizeVector;
}

void ViewerDataHandler::prepareCSVFile()
{
	if (!plotHandler->getSettings().shouldLog)
		return;

	std::vector<std::string> headerNames;

	for (auto& [name, plot] : *plotGroupHandler->getActiveGroup())
	{
		if (!plot->getVisibility())
			continue;

		for (auto& [name, ser] : plot->getSeriesMap())
			headerNames.push_back(name);
	}
	csvStreamer->prepareFile(plotHandler->getSettings().logFilePath);
	csvStreamer->createHeader(headerNames);
}