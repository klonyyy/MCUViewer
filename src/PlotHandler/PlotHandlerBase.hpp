#ifndef _PLOTHANDLERBASE_HPP
#define _PLOTHANDLERBASE_HPP

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#include "Plot.hpp"
#include "ScrollingBuffer.hpp"

class PlotHandlerBase
{
   public:
	std::shared_ptr<Plot> addPlot(const std::string& name);
	bool removePlot(const std::string& name);
	bool renamePlot(const std::string& oldName, const std::string& newName);
	bool removeAllPlots();
	std::shared_ptr<Plot> getPlot(std::string name);
	bool eraseAllPlotData();
	uint32_t getVisiblePlotsCount() const;
	uint32_t getPlotsCount() const;
	bool checkIfPlotExists(const std::string& name) const;
	void setMaxPoints(uint32_t maxPoints);

	class iterator
	{
	   public:
		using iterator_category = std::forward_iterator_tag;
		explicit iterator(std::map<std::string, std::shared_ptr<Plot>>::iterator iter);
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
	std::map<std::string, std::shared_ptr<Plot>> plotsMap;
};

#endif