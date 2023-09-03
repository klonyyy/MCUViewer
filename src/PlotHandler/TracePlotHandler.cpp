#include "TracePlotHandler.hpp"

#include "TraceReader.hpp"

TracePlotHandler::TracePlotHandler(std::atomic<bool>& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger) : PlotHandlerBase(done, mtx, logger)
{
	traceDevice = std::make_unique<StlinkTraceDevice>(logger);
	traceReader = std::make_unique<TraceReader>(traceDevice, logger);

	const uint32_t colors[] = {4294967040, 4294960666, 4294954035, 4294947661, 4294941030, 4294934656, 4294928025, 4294921651, 4294915020, 4294908646, 4294902015};

	for (uint32_t i = 0; i < channels; i++)
	{
		std::string name = std::string("CH" + std::to_string(i));
		plotsMap[name] = std::make_shared<Plot>(name);

		auto newVar = std::make_shared<Variable>(name);
		newVar->setColor(colors[i]);
		traceVars[name] = newVar;

		plotsMap[name]->addSeries(*newVar);
		plotsMap[name]->setDomain(Plot::Domain::DIGITAL);
		plotsMap[name]->setAlias("CH" + std::to_string(i));
	}

	dataHandle = std::thread(&TracePlotHandler::dataHandler, this);
}
TracePlotHandler::~TracePlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
}

TracePlotHandler::Settings TracePlotHandler::getSettings() const
{
	return traceSettings;
}

void TracePlotHandler::setSettings(const Settings& settings)
{
	traceReader->setCoreClockFrequency(settings.coreFrequency);
	traceReader->setTraceFrequency(settings.tracePrescaler);
	setMaxPoints(settings.maxPoints);
	traceSettings = settings;
}

std::map<std::string, uint32_t> TracePlotHandler::getTraceIndicators() const
{
	auto indicators = traceReader->getTraceIndicators();
	indicators.at("error frames in view") = errorFrameTimestamps.size();
	return indicators;
}

std::string TracePlotHandler::getLastReaderError() const
{
	return traceReader->getLastErrorMsg();
}

void TracePlotHandler::setTriggerChannel(int32_t triggerChannel)
{
	traceSettings.triggerChannel = triggerChannel;
}

int32_t TracePlotHandler::getTriggerChannel() const
{
	return traceSettings.triggerChannel;
}

void TracePlotHandler::dataHandler()
{
	uint32_t cnt = 0;

	while (!done)
	{
		if (viewerState == state::RUN)
		{
			if (!traceReader->isValid())
			{
				logger->error("Trace invalid, stopping!");
				viewerState.store(state::STOP);
				stateChangeOrdered.store(true);
			}

			double timestamp;
			std::array<uint32_t, 10> traces{};
			if (!traceReader->readTrace(timestamp, traces))
				continue;

			uint32_t i = 0;

			time += timestamp;

			double oldestTimestamp = plotsMap.begin()->second->getTimeSeries().getOldestValue();

			if (errorFrameSinceLastPoint != traceReader->getTraceIndicators().at("error frames total"))
				errorFrameTimestamps.push_back(time);

			while (errorFrameTimestamps.size() && errorFrameTimestamps.front() < oldestTimestamp)
				errorFrameTimestamps.pop_front();

			errorFrameSinceLastPoint = traceReader->getTraceIndicators().at("error frames total");

			for (auto& [key, plot] : plotsMap)
			{
				if (!plot->getVisibility())
				{
					i++;
					continue;
				}

				Plot::Series* ser = plot->getSeriesMap().begin()->second.get();
				double newPoint = 0.0;

				if (plot->getDomain() == Plot::Domain::DIGITAL)
					newPoint = traces[i] == 0xaa ? 1.0 : 0.0;
				else if (plot->getDomain() == Plot::Domain::ANALOG)
					newPoint = *(float*)&traces[i];

				if (traceTriggered == false && i == traceSettings.triggerChannel && newPoint > traceSettings.triggerLevel)
				{
					logger->info("Trigger!");
					traceTriggered = true;
					timestamp = 0;
					cnt = 0;
				}

				/* thread-safe part */
				std::lock_guard<std::mutex> lock(*mtx);
				plot->addPoint(ser->var->getName(), newPoint);
				plot->addTimePoint(time);
				i++;
			}
			if (traceTriggered && cnt++ >= (traceSettings.maxPoints * 0.9))
			{
				logger->info("After-trigger trace collcted. Stopping.");
				viewerState.store(state::STOP);
				stateChangeOrdered.store(true);
			}
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if (stateChangeOrdered.load())
		{
			if (viewerState == state::RUN)
			{
				std::array<bool, 32> activeChannels{};

				uint32_t i = 0;
				for (auto& [key, plot] : plotsMap)
					activeChannels[i++] = plot->getVisibility();

				if (traceReader->startAcqusition(activeChannels))
					time = 0;
				else
					viewerState = state::STOP;
			}
			else
			{
				traceReader->stopAcqusition();
				traceTriggered = false;
			}
			stateChangeOrdered = false;
		}
	}
	logger->info("Exiting trace plot handler thread");
}
