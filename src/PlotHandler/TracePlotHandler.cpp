#include "TracePlotHandler.hpp"

#include <algorithm>
#include <memory>
#include <string>

#include "TraceReader.hpp"

TracePlotHandler::TracePlotHandler(std::atomic<bool>& done, std::mutex* mtx, spdlog::logger* logger) : PlotHandlerBase(done, mtx, logger)
{
	traceReader = std::make_unique<TraceReader>(logger);
	initPlots();
	dataHandle = std::thread(&TracePlotHandler::dataHandler, this);
}
TracePlotHandler::~TracePlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
}

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

		plotsMap[name]->addSeries(*newVar);
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
	traceReader->setCoreClockFrequency(settings.coreFrequency);
	traceReader->setTraceFrequency(settings.tracePrescaler);
	traceReader->setTraceShouldReset(settings.shouldReset);
	traceReader->setTraceTimeout(settings.timeout);
	setMaxPoints(settings.maxPoints);
	this->settings = settings;
}

TraceReader::TraceIndicators TracePlotHandler::getTraceIndicators() const
{
	auto indicators = traceReader->getTraceIndicators();
	indicators.errorFramesInView = errorFrames.size();
	indicators.delayedTimestamp3InView = delayed3Frames.size();
	return indicators;
}

std::vector<double> TracePlotHandler::getErrorTimestamps()
{
	return errorFrames.getVector();
}

std::vector<double> TracePlotHandler::getDelayed3Timestamps()
{
	return delayed3Frames.getVector();
}

std::string TracePlotHandler::getLastReaderError() const
{
	auto traceReaderMsg = traceReader->getLastErrorMsg();
	return traceReaderMsg.empty() ? lastErrorMsg : traceReaderMsg;
}

void TracePlotHandler::setTriggerChannel(int32_t triggerChannel)
{
	settings.triggerChannel = triggerChannel;
}

int32_t TracePlotHandler::getTriggerChannel() const
{
	return settings.triggerChannel;
}

void TracePlotHandler::setDebugProbe(std::shared_ptr<ITraceProbe> probe)
{
	traceReader->changeDevice(probe);
}

ITraceProbe::TraceProbeSettings TracePlotHandler::getProbeSettings() const
{
	return probeSettings;
}

void TracePlotHandler::setProbeSettings(const ITraceProbe::TraceProbeSettings& settings)
{
	probeSettings = settings;
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

void TracePlotHandler::dataHandler()
{
	uint32_t cnt = 0;
	double time = 0.0;
	std::vector<double> csvValues;
	csvValues.reserve(channels);

	while (!done)
	{
		if (viewerState == state::RUN)
		{
			if (!traceReader->isValid())
			{
				logger->error("Trace invalid, stopping!");
				viewerState = state::STOP;
				stateChangeOrdered = true;
			}

			double timestamp;
			std::array<uint32_t, channels> traces{};
			if (!traceReader->readTrace(timestamp, traces))
				continue;

			time += timestamp;

			double oldestTimestamp = plotsMap.begin()->second->getXAxisSeries()->getOldestValue();
			auto indicators = traceReader->getTraceIndicators();

			errorFrames.handle(time, oldestTimestamp, indicators.errorFramesTotal);
			delayed3Frames.handle(time, oldestTimestamp, indicators.delayedTimestamp3);

			csvValues.clear();
			uint32_t i = 0;
			for (auto& [key, plot] : plotsMap)
			{
				if (!plot->getVisibility())
				{
					i++;
					continue;
				}

				Plot::Series* ser = plot->getSeriesMap().begin()->second.get();
				double newPoint = getDoubleValue(*plot, traces[i]);

				if (traceTriggered == false && i == static_cast<uint32_t>(settings.triggerChannel) && newPoint > settings.triggerLevel)
				{
					logger->info("Trigger!");
					traceTriggered = true;
					timestamp = 0;
					cnt = 0;
				}

				csvValues.push_back(newPoint);

				/* thread-safe part */
				std::lock_guard<std::mutex> lock(*mtx);
				plot->addPoint(ser->var->getName(), newPoint);
				plot->addTimePoint(time);
				i++;
			}

			if (settings.shouldLog)
				csvStreamer->writeLine(time, csvValues);

			if (traceTriggered && cnt++ >= (settings.maxPoints * 0.9))
			{
				logger->info("After-trigger trace collcted. Stopping.");
				viewerState = state::STOP;
				stateChangeOrdered = true;
			}

			if (errorFrames.size() > maxAllowedViewportErrors)
			{
				lastErrorMsg = "Too many error frames!";
				logger->error("Too many error frames. Please modify your clock and prescaler settings. Stopping.");
				viewerState = state::STOP;
				stateChangeOrdered = true;
			}

			if (delayed3Frames.size() > maxAllowedViewportErrors)
			{
				lastErrorMsg = "Too many delayed timestamp 3 frames!";
				logger->error("Too many delayed timestamp 3 frames. Please modify your clock and prescaler settings or limit the logged channels. Stopping.");
				viewerState = state::STOP;
				stateChangeOrdered = true;
			}
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if (stateChangeOrdered)
		{
			if (viewerState == state::RUN)
			{
				std::array<bool, 32> activeChannels{};

				size_t i = 0;
				for (auto& [key, plot] : plotsMap)
					activeChannels[i++] = plot->getVisibility();

				errorFrames.reset();
				delayed3Frames.reset();
				lastErrorMsg = "";

				prepareCSVFile();

				if (traceReader->startAcqusition(probeSettings, activeChannels))
					time = 0;
				else
					viewerState = state::STOP;
			}
			else
			{
				traceReader->stopAcqusition();
				if (settings.shouldLog)
					csvStreamer->finishLogging();
				traceTriggered = false;
			}
			stateChangeOrdered = false;
		}
	}
	logger->info("Exiting trace plot handler thread");
}

void TracePlotHandler::prepareCSVFile()
{
	if (!settings.shouldLog)
		return;

	std::vector<std::string> headerNames;

	size_t i = 0;
	for (auto& [key, plot] : plotsMap)
	{
		if (plot->getVisibility())
			headerNames.push_back(std::string("CH") + std::to_string(i));
		i++;
	}

	csvStreamer->prepareFile(settings.logFilePath);
	csvStreamer->createHeader(headerNames);
}