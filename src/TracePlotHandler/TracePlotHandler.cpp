#include "TracePlotHandler.hpp"

#include "StlinkTraceReader.hpp"

TracePlotHandler::TracePlotHandler(bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger) : done(done), mtx(mtx), logger(logger)
{
	dataHandle = std::thread(&TracePlotHandler::dataHandler, this);
	traceReader = std::make_unique<StlinkTraceReader>();
}
TracePlotHandler::~TracePlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
}

void TracePlotHandler::addPlot(const std::string& name)
{
	plotsMap[name] = std::make_shared<Plot>(name);
}

std::shared_ptr<Plot> TracePlotHandler::getPlot(std::string name)
{
	return plotsMap.at(name);
}

bool TracePlotHandler::eraseAllPlotData()
{
	if (plotsMap.empty())
		return false;

	for (auto& plot : plotsMap)
		if (plot.second != nullptr)
			plot.second->erase();

	return true;
}

void TracePlotHandler::setViewerState(state state)
{
	if (state == viewerState)
		return;
	stateChangeOrdered = true;
	viewerStateTemp = state;
}

TracePlotHandler::state TracePlotHandler::getViewerState() const
{
	return viewerState;
}

void TracePlotHandler::dataHandler()
{
	uint32_t timer = 0;
	std::array<bool, 10> lastTraces;

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

			lastTraces = traces;
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if (stateChangeOrdered)
		{
			viewerState = viewerStateTemp;

			if (viewerState == state::RUN)
			{
				if (traceReader->startAcqusition())
					timer = 0;
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

void TracePlotHandler::setMaxPoints(uint32_t maxPoints)
{
	for (auto& [name, plt] : plotsMap)
	{
		for (auto& [name, ser] : plt->getSeriesMap())
			ser->buffer->setMaxSize(maxPoints);
		plt->getTimeSeries().setMaxSize(maxPoints);
	}
}

TracePlotHandler::iterator::iterator(std::map<std::string, std::shared_ptr<Plot>>::iterator iter)
	: m_iter(iter)
{
}

TracePlotHandler::iterator& TracePlotHandler::iterator::operator++()
{
	++m_iter;
	return *this;
}

TracePlotHandler::iterator TracePlotHandler::iterator::operator++(int)
{
	iterator tmp = *this;
	++(*this);
	return tmp;
}

bool TracePlotHandler::iterator::operator==(const iterator& other) const
{
	return m_iter == other.m_iter;
}

bool TracePlotHandler::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}

std::shared_ptr<Plot> TracePlotHandler::iterator::operator*()
{
	return m_iter->second;
}

TracePlotHandler::iterator TracePlotHandler::begin()
{
	return iterator(plotsMap.begin());
}

TracePlotHandler::iterator TracePlotHandler::end()
{
	return iterator(plotsMap.end());
}