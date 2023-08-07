#include "TracePlotHandler.hpp"

#include "StlinkTraceReader.hpp"

TracePlotHandler::TracePlotHandler(bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger) : PlotHandlerBase(done, mtx, logger)
{
	dataHandle = std::thread(&TracePlotHandler::dataHandler, this);
	traceReader = std::make_unique<StlinkTraceReader>();

	for (uint32_t i = 0; i < channels; i++)
	{
		std::string name = std::string("CH" + std::to_string(i));
		plotsMap[name] = std::make_shared<Plot>(name);

		auto newVar = std::make_shared<Variable>(name);
		newVar->setColor(25251254);
		traceVars[name] = newVar;

		plotsMap[name]->addSeries(*newVar);
	}
}
TracePlotHandler::~TracePlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
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
			std::this_thread::sleep_for(std::chrono::microseconds(100));

			double timestamp;
			std::array<bool, 10> traces{};
			traceReader->readTrace(timestamp, traces);

			uint32_t i = 0;

			time += timestamp;

			for (auto& [key, plot] : plotsMap)
			{
				if (!plot->getVisibility())
					continue;

				Plot::Series* ser = plot->getSeriesMap().begin()->second.get();

				/* thread-safe part */
				std::lock_guard<std::mutex> lock(*mtx);
				plot->addPoint(ser->var->getName(), (double)traces[i++]);
				plot->addTimePoint(time);
			}
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if (stateChangeOrdered)
		{
			viewerState = viewerStateTemp;

			if (viewerState == state::RUN)
			{
				if (traceReader->startAcqusition())
					time = 0;
				else
				{
					viewerState = state::STOP;
					viewerStateTemp = state::STOP;
				}
			}
			else
				traceReader->stopAcqusition();
			stateChangeOrdered = false;
		}
	}
}
