#include "PlotHandler.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>

#include "JlinkHandler.hpp"
#include "StlinkHandler.hpp"

PlotHandler::PlotHandler(std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger) : PlotHandlerBase(done, mtx, logger)
{
	dataHandle = std::thread(&PlotHandler::dataHandler, this);
	varReader = std::make_unique<TargetMemoryHandler>();
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

void PlotHandler::setDebugProbe(std::shared_ptr<IDebugProbe> probe, const std::string& serialNumber)
{
	probeSettings.serialNumber = serialNumber;
	varReader->changeDevice(probe);
}

void PlotHandler::setTargetDevice(const std::string& deviceName)
{
	probeSettings.device = deviceName;
}

void PlotHandler::setProbeMode(const IDebugProbe::Mode mode)
{
	probeSettings.mode = mode;
}

void PlotHandler::dataHandler()
{
	uint32_t timer = 0;
	double lastT = 0.0;
	double sum = 0.0;

	while (!done)
	{
		if (viewerState == state::RUN)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			auto finish = std::chrono::steady_clock::now();
			double t = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();

			if (probeSettings.mode == IDebugProbe::Mode::HSS)
			{
				auto maybeEntry = varReader->readSingleEntry();

				if (!maybeEntry.has_value())
					continue;

				auto entry = maybeEntry.value();

				for (auto& [key, plot] : plotsMap)
				{
					if (!plot->getVisibility())
						continue;

					std::lock_guard<std::mutex> lock(*mtx);
					/* thread-safe part */
					for (auto& [name, ser] : plot->getSeriesMap())
					{
						double value = varReader->castToProperType(entry.second[ser->var->getAddress()], ser->var->getType());
						ser->var->setValue(value);
						plot->addPoint(name, value);
					}
					plot->addTimePoint(entry.first);
				}

				timer++;
				sum += (t - lastT);
				averageSamplingFrequency = sum / timer;
				lastT = t;
			}

			else if (t > (1.0f / settings.sampleFrequencyHz) * timer)
			{
				for (auto& [key, plot] : plotsMap)
				{
					if (!plot->getVisibility())
						continue;

					/* this part consumes most of the thread time */
					for (auto& [name, ser] : plot->getSeriesMap())
					{
						bool result = false;
						auto value = varReader->getValue(ser->var->getAddress(), ser->var->getType(), result);

						if (result)
							ser->var->setValue(value);
					}

					/* thread-safe part */
					std::lock_guard<std::mutex> lock(*mtx);
					for (auto& [name, ser] : plot->getSeriesMap())
						plot->addPoint(name, ser->var->getValue());
					plot->addTimePoint(t);
				}
				timer++;
				sum += (t - lastT);
				averageSamplingFrequency = sum / timer;
				lastT = t;
			}
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if (stateChangeOrdered)
		{
			if (viewerState == state::RUN)
			{
				auto addressSizeVector = createAddressSizeVector();

				if (varReader->start(probeSettings.serialNumber, addressSizeVector, settings.sampleFrequencyHz, probeSettings.mode, probeSettings.device))
				{
					timer = 0;
					lastT = 0.0;
					sum = 0.0;
					start = std::chrono::steady_clock::now();
				}
				else
					viewerState = state::STOP;
			}
			else
				varReader->stop();
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