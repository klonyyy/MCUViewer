#ifndef _TRACEPLOTHANDLER_HPP
#define _TRACEPLOTHANDLER_HPP

#include <map>

#include "ITraceReader.hpp"
#include "Plot.hpp"
#include "spdlog/spdlog.h"

class TracePlotHandler
{
   public:
	enum class state
	{
		STOP = 0,
		RUN = 1,
	};

	TracePlotHandler(bool& done, std::mutex* mtx, std::shared_ptr<spdlog::logger> logger);
	~TracePlotHandler();

	void addPlot(const std::string& name);
	std::shared_ptr<Plot> getPlot(std::string name);
	bool eraseAllPlotData();
	void setViewerState(state state);
	state getViewerState() const;
	uint32_t getPlotsCount() const;
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

   private:
	void dataHandler();

   private:
	bool& done;
	state viewerState = state::STOP;
	state viewerStateTemp = state::STOP;
	std::unique_ptr<ITraceReader> traceReader;

	std::map<std::string, std::shared_ptr<Plot>> plotsMap;

	std::mutex* mtx;
	std::thread dataHandle;
	bool stateChangeOrdered = false;

	double time = 0.0;

	std::shared_ptr<spdlog::logger> logger;
};

#endif