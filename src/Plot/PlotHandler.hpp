#ifndef _PLOTHANDLER_HPP
#define _PLOTHANDLER_HPP

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#include "Plot.hpp"
#include "ScrollingBuffer.hpp"
#include "VarReader.hpp"

class PlotHandler
{
   public:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};

	class iterator
	{
	   public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = std::shared_ptr<Plot>;
		using difference_type = std::ptrdiff_t;

		iterator(std::map<std::string, std::shared_ptr<Plot>>::iterator iter)
			: m_iter(iter)
		{
		}

		iterator& operator++()
		{
			++m_iter;
			return *this;
		}

		iterator operator++(int)
		{
			iterator tmp = *this;
			++(*this);
			return tmp;
		}

		bool operator==(const iterator& other) const
		{
			return m_iter == other.m_iter;
		}

		bool operator!=(const iterator& other) const
		{
			return !(*this == other);
		}

		std::shared_ptr<Plot> operator*()
		{
			return m_iter->second;
		}

	   private:
		std::map<std::string, std::shared_ptr<Plot>>::iterator m_iter;
	};

	PlotHandler(bool& done, std::mutex* mtx);
	~PlotHandler();

	void addPlot(std::string name);
	bool removePlot(std::string name);
	bool renamePlot(std::string oldName, std::string newName);
	bool removeAllPlots();
	std::shared_ptr<Plot> getPlot(std::string name);
	bool eraseAllPlotData();
	void setViewerState(state state);
	bool getViewerState();
	uint32_t getVisiblePlotsCount();
	uint32_t getPlotsCount();
	bool writeSeriesValue(Variable& var, const float value);

	iterator begin()
	{
		return iterator(plotsMap.begin());
	}

	iterator end()
	{
		return iterator(plotsMap.end());
	}

   private:
	bool& done;
	state viewerState = state::STOP;
	state viewerStateTemp = state::STOP;
	std::unique_ptr<VarReader> varReader;

	std::mutex* mtx;

	std::map<std::string, std::shared_ptr<Plot>> plotsMap;

	std::thread dataHandle;

	bool stateChangeOrdered = false;

	std::chrono::time_point<std::chrono::steady_clock> start;

	static constexpr uint32_t maxVariables = 100;

	void dataHandler();
};

#endif