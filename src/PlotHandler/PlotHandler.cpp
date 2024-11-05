#include "PlotHandler.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>

#include "JlinkDebugProbe.hpp"
#include "StlinkDebugProbe.hpp"

PlotHandler::PlotHandler(std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger) : PlotHandlerBase(done, mtx, logger)
{
	dataHandle = std::thread(&PlotHandler::dataHandler, this);
	varReader = std::make_unique<MemoryReader>();
}
PlotHandler::~PlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
}

PlotHandler::Settings PlotHandler::getSettings() const
{
	return settings;
}

void PlotHandler::setSettings(const Settings& newSettings)
{
	settings = newSettings;
	setMaxPoints(settings.maxPoints);
}

bool PlotHandler::writeSeriesValue(Variable& var, double value)
{
	std::lock_guard<std::mutex> lock(*mtx);
	return varReader->setValue(var, value);
}

std::string PlotHandler::getLastReaderError() const
{
	return varReader->getLastErrorMsg();
}

void PlotHandler::setDebugProbe(std::shared_ptr<IDebugProbe> probe)
{
	varReader->changeDevice(probe);
}

IDebugProbe::DebugProbeSettings PlotHandler::getProbeSettings() const
{
	return probeSettings;
}

void PlotHandler::setProbeSettings(const IDebugProbe::DebugProbeSettings& settings)
{
	probeSettings = settings;
}

void PlotHandler::dataHandler()
{
	std::chrono::time_point<std::chrono::steady_clock> start;
	uint32_t timer = 0;
	double lastT = 0.0;
	std::vector<double> csvValues;
	csvValues.reserve(maxVariablesOnSinglePlot);

	while (!done)
	{
		if (viewerState == state::RUN)
		{
			auto finish = std::chrono::steady_clock::now();
			double period = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
			csvValues.clear();

			if (probeSettings.mode == IDebugProbe::Mode::HSS)
			{
				auto maybeEntry = varReader->readSingleEntry();

				if (!maybeEntry.has_value())
					continue;

				auto [timestamp, values] = maybeEntry.value();

				for (auto& [key, plot] : plotsMap)
				{
					if (!plot->getVisibility())
						continue;

					std::lock_guard<std::mutex> lock(*mtx);
					/* thread-safe part */
					for (auto& [name, ser] : plot->getSeriesMap())
					{
						auto var = ser->var;
						var->setRawValueAndTransform(values[var->getAddress()]);
						plot->addPoint(name, var->getValue());
						csvValues.push_back(var->getValue());
					}
					plot->addTimePoint(timestamp);
				}

				if (settings.shouldLog)
					csvStreamer->writeLine(period, csvValues);
				/* filter sampling frequency */
				averageSamplingPeriod = samplingPeriodFilter.filter((period - lastT));
				lastT = period;
				timer++;
			}

			else if (period > ((1.0 / settings.sampleFrequencyHz) * timer))
			{
				for (auto& [key, plot] : plotsMap)
				{
					if (!plot->getVisibility())
						continue;

					/* this part consumes most of the thread time */
					for (auto& [name, ser] : plot->getSeriesMap())
					{
						bool result = false;
						uint32_t valueRaw = varReader->getValue(ser->var->getAddress(), ser->var->getSize(), result);
						if (result)
							ser->var->setRawValueAndTransform(valueRaw);
					}

					/* thread-safe part */
					std::lock_guard<std::mutex> lock(*mtx);
					for (auto& [name, ser] : plot->getSeriesMap())
					{
						plot->addPoint(name, ser->var->getValue());
						csvValues.push_back(ser->var->getValue());
					}
					plot->addTimePoint(period);
				}

				if (settings.shouldLog)
					csvStreamer->writeLine(period, csvValues);
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

				if (varReader->start(probeSettings, addressSizeVector, settings.sampleFrequencyHz))
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
				if (settings.shouldLog)
					csvStreamer->finishLogging();
			}
			stateChangeOrdered = false;
		}
	}
}

std::vector<std::pair<uint32_t, uint8_t>> PlotHandler::createAddressSizeVector()
{
	std::vector<std::pair<uint32_t, uint8_t>> addressSizeVector;

	for (auto& [key, plot] : plotsMap)
	{
		if (!plot->getVisibility())
			continue;

		for (auto& [name, ser] : plot->getSeriesMap())
			addressSizeVector.push_back({ser->var->getAddress(), ser->var->getSize()});
	}

	return addressSizeVector;
}

void PlotHandler::prepareCSVFile()
{
	if (!settings.shouldLog)
		return;

	std::vector<std::string> headerNames;

	for (auto& [key, plot] : plotsMap)
	{
		if (!plot->getVisibility())
			continue;

		for (auto& [name, ser] : plot->getSeriesMap())
			headerNames.push_back(name);
	}
	csvStreamer->prepareFile(settings.logFilePath);
	csvStreamer->createHeader(headerNames);
}