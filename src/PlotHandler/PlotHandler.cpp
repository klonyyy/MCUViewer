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
	varReader = std::make_unique<TargetMemoryHandler>(logger);
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
	/* TODO lock with timeout as we dont really care if we get it every cycle */
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

void PlotHandler::dataHandler()
{
	uint32_t timer = 0;

	while (!done)
	{
		if (viewerState == state::RUN)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			auto finish = std::chrono::steady_clock::now();
			double t = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();

			if (t > (settings.samplePeriod * timer) / 1000.0f)
			{
				for (auto& [key, plot] : plotsMap)
				{
					if (!plot->getVisibility())
						continue;

					/* this part consumes most of the thread time */
					for (auto& [name, ser] : plot->getSeriesMap())
						ser->var->setValue(varReader->getValue(ser->var->getAddress(), ser->var->getType()));

					/* thread-safe part */
					std::lock_guard<std::mutex> lock(*mtx);
					for (auto& [name, ser] : plot->getSeriesMap())
						plot->addPoint(name, ser->var->getValue());
					plot->addTimePoint(t);
				}
				timer++;
			}
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if (stateChangeOrdered)
		{
			if (viewerState == state::RUN)
			{
				if (varReader->start(probeSettings.serialNumber, probeSettings.device))
				{
					timer = 0;
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