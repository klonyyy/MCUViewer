#include "PlotHandler.hpp"

#include <algorithm>
#include <array>

PlotHandler::PlotHandler(bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger) : done(done), mtx(mtx), logger(logger)
{
	dataHandle = std::thread(&PlotHandler::dataHandler, this);
	varReader = std::make_unique<VarReader>(logger);
}
PlotHandler::~PlotHandler()
{
	if (dataHandle.joinable())
		dataHandle.join();
}

void PlotHandler::addPlot(const std::string& name)
{
	plotsMap[name] = std::make_shared<Plot>(name);
}
bool PlotHandler::removePlot(const std::string& name)
{
	plotsMap.erase(name);
	return true;
}

bool PlotHandler::renamePlot(const std::string& oldName, const std::string& newName)
{
	auto plt = plotsMap.extract(oldName);
	plt.key() = newName;
	plotsMap.insert(std::move(plt));
	plotsMap[newName]->setName(newName);
	return true;
}

bool PlotHandler::removeAllPlots()
{
	plotsMap.clear();
	return true;
}

std::shared_ptr<Plot> PlotHandler::getPlot(std::string name)
{
	return plotsMap.at(name);
}

bool PlotHandler::eraseAllPlotData()
{
	if (plotsMap.empty())
		return false;

	for (auto& plot : plotsMap)
		if (plot.second != nullptr)
			plot.second->erase();

	return true;
}

void PlotHandler::setViewerState(state state)
{
	if (state == viewerState)
		return;
	stateChangeOrdered = true;
	viewerStateTemp = state;
}

PlotHandler::state PlotHandler::getViewerState() const
{
	return viewerState;
}

uint32_t PlotHandler::getVisiblePlotsCount() const
{
	return std::count_if(plotsMap.begin(), plotsMap.end(), [](const auto& pair)
						 { return pair.second->getVisibility(); });
}
uint32_t PlotHandler::getPlotsCount() const
{
	return plotsMap.size();
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

			if (t > (samplePeriodMs * timer) / 1000.0f)
			{
				for (auto& [key, plot] : plotsMap)
				{
					if (!plot->getVisibility())
						continue;

					/* this part consumes most of the thread time */
					for (auto& [name, ser] : plot->getSeriesMap())
						ser->var->setValue(varReader->getDouble(ser->var->getAddress(), ser->var->getType()));

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
			viewerState = viewerStateTemp;

			if (viewerState == state::RUN)
			{
				if (varReader->start())
				{
					timer = 0;
					start = std::chrono::steady_clock::now();
				}
				else
				{
					viewerState = state::STOP;
					viewerStateTemp = state::STOP;
				}
			}
			else
				varReader->stop();
			stateChangeOrdered = false;
		}
	}
}

bool PlotHandler::writeSeriesValue(Variable& var, double value)
{
	std::lock_guard<std::mutex> lock(*mtx);
	return varReader->setValue(var, value);
}

bool PlotHandler::checkIfPlotExists(const std::string&& name) const
{
	return plotsMap.find(name) != plotsMap.end();
}

std::string PlotHandler::getLastReaderError() const
{
	return varReader->getLastErrorMsg();
}

void PlotHandler::setSamplePeriod(uint32_t period)
{
	samplePeriodMs = period;
}

void PlotHandler::setMaxPoints(uint32_t maxPoints)
{
	for (auto& [name, plt] : plotsMap)
	{
		for (auto& [name, ser] : plt->getSeriesMap())
			ser->buffer->setMaxSize(maxPoints);
		plt->getTimeSeries().setMaxSize(maxPoints);
	}
}

uint32_t PlotHandler::getSamplePeriod() const
{
	return samplePeriodMs;
}

PlotHandler::iterator::iterator(std::map<std::string, std::shared_ptr<Plot>>::iterator iter)
	: m_iter(iter)
{
}

PlotHandler::iterator& PlotHandler::iterator::operator++()
{
	++m_iter;
	return *this;
}

PlotHandler::iterator PlotHandler::iterator::operator++(int)
{
	iterator tmp = *this;
	++(*this);
	return tmp;
}

bool PlotHandler::iterator::operator==(const iterator& other) const
{
	return m_iter == other.m_iter;
}

bool PlotHandler::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}

std::shared_ptr<Plot> PlotHandler::iterator::operator*()
{
	return m_iter->second;
}

PlotHandler::iterator PlotHandler::begin()
{
	return iterator(plotsMap.begin());
}

PlotHandler::iterator PlotHandler::end()
{
	return iterator(plotsMap.end());
}
