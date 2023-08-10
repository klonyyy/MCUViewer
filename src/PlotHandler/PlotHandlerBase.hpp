#ifndef _PLOTHANDLERBASE_HPP
#define _PLOTHANDLERBASE_HPP

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#include "Plot.hpp"
#include "ScrollingBuffer.hpp"
#include "StlinkHandler.hpp"
#include "spdlog/spdlog.h"

class PlotHandlerBase
{
   public:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};

	PlotHandlerBase(bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
	virtual ~PlotHandlerBase() = default;

	void addPlot(const std::string& name);
	bool removePlot(const std::string& name);
	bool renamePlot(const std::string& oldName, const std::string& newName);
	bool removeAllPlots();
	std::shared_ptr<Plot> getPlot(std::string name);
	bool eraseAllPlotData();
	void setViewerState(state state);
	state getViewerState() const;
	uint32_t getVisiblePlotsCount() const;
	uint32_t getPlotsCount() const;
	bool writeSeriesValue(Variable& var, double value);
	bool checkIfPlotExists(const std::string&& name) const;
	void setMaxPoints(uint32_t maxPoints);

	class iterator
	{
	   public:
		using iterator_category = std::forward_iterator_tag;
		iterator(std::map<std::string, std::shared_ptr<Plot>>::iterator iter);
		iterator& operator++();
		iterator operator++(int);
		bool operator==(const iterator& other) const;
		bool operator!=(const iterator& other) const;
		std::shared_ptr<Plot> operator*();

	   private:
		std::map<std::string, std::shared_ptr<Plot>>::iterator m_iter;
	};

	iterator begin();
	iterator end();

   protected:
	bool& done;
	state viewerState = state::STOP;
	state viewerStateTemp = state::STOP;
	std::map<std::string, std::shared_ptr<Plot>> plotsMap;
	std::mutex* mtx;
	std::thread dataHandle;
	bool stateChangeOrdered = false;
	std::shared_ptr<spdlog::logger> logger;
};

#endif