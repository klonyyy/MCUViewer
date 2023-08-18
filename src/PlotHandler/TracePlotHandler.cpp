#include "TracePlotHandler.hpp"

#include "TraceReader.hpp"

TracePlotHandler::TracePlotHandler(std::atomic<bool>& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger) : PlotHandlerBase(done, mtx, logger)
{
	traceDevice = std::make_unique<StlinkTraceDevice>(logger);
	traceReader = std::make_unique<TraceReader>(traceDevice, logger);

	for (uint32_t i = 0; i < channels; i++)
	{
		std::string name = std::string("CH" + std::to_string(i));
		plotsMap[name] = std::make_shared<Plot>(name);

		auto newVar = std::make_shared<Variable>(name);
		newVar->setColor(25251254);
		traceVars[name] = newVar;

		plotsMap[name]->addSeries(*newVar);
		plotsMap[name]->setDomain(Plot::Domain::DIGITAL);
	}

	dataHandle = std::thread(&TracePlotHandler::dataHandler, this);
}
TracePlotHandler::~TracePlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
}

TracePlotHandler::TraceSettings TracePlotHandler::getTraceSettings() const
{
	return {.coreFrequency = traceReader->getCoreClockFrequency(), .traceFrequency = traceReader->getTraceFrequency()};
}

void TracePlotHandler::setTraceSettings(const TraceSettings& settings)
{
	traceReader->setCoreClockFrequency(settings.coreFrequency);
	traceReader->setTraceFrequency(settings.traceFrequency);
	traceSettings = settings;
}

std::map<const char*, uint32_t> TracePlotHandler::getTraceIndicators() const
{
	return traceReader->getTraceIndicators();
}

std::string TracePlotHandler::getLastReaderError() const
{
	return traceReader->getLastErrorMsg();
}

void TracePlotHandler::dataHandler()
{
	while (!done)
	{
		if (viewerState == state::RUN)
		{
			if (!traceReader->isValid())
			{
				logger->warn("TRACE INVALID, STOPPING");
				viewerState.store(state::STOP);
				stateChangeOrdered.store(true);
			}

			std::this_thread::sleep_for(std::chrono::microseconds(100));

			double timestamp;
			std::array<uint32_t, 10> traces{};
			if (!traceReader->readTrace(timestamp, traces))
				continue;

			uint32_t i = 0;

			time += timestamp;

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

				/* thread-safe part */
				std::lock_guard<std::mutex>
					lock(*mtx);
				plot->addPoint(ser->var->getName(), newPoint);
				plot->addTimePoint(time);
				i++;
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
				traceReader->stopAcqusition();
			stateChangeOrdered = false;
		}
	}
}
