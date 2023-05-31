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

	PlotHandler(bool& done, std::mutex* mtx);
	~PlotHandler();

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
	std::string getLastReaderError() const;

	void setSamplePeriod(uint32_t period);
	uint32_t getSamplePeriod() const;

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
	std::unique_ptr<VarReader> varReader;

	std::map<std::string, std::shared_ptr<Plot>> plotsMap;

	std::mutex* mtx;
	std::thread dataHandle;
	bool stateChangeOrdered = false;

	std::chrono::time_point<std::chrono::steady_clock> start;
	float samplePeriodMs = 1;
};

#endif