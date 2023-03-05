#ifndef _PLOT_HPP
#define _PLOT_HPP

#include <functional>
#include <map>
#include <mutex>
#include <thread>

#include "ScrollingBuffer.hpp"
#include "Variable.hpp"
class Plot
{
   public:
	struct Series
	{
		Variable::type type;
		std::string* seriesName;
		std::unique_ptr<ScrollingBuffer<float>> buffer;
	};

	Plot(std::string name);
	~Plot();
	std::string getName() const;
	bool addSeries(std::string* name, uint32_t address);
	bool addSeries(Variable& var);
	std::shared_ptr<Plot::Series> getSeries(uint32_t address);
	std::map<uint32_t, std::shared_ptr<Plot::Series>>& getSeriesMap();
	ScrollingBuffer<float>& getTimeSeries();
	bool removeVariable(uint32_t address);
	bool removeAllVariables();
	std::vector<uint32_t> getVariableAddesses();
	std::vector<Variable::type> getVariableTypes();
	bool addPoint(float t, uint32_t address, float value);
	bool addTimePoint(float t);
	void erase();

   private:
	std::mutex mtx;
	std::string name;
	std::map<uint32_t, std::shared_ptr<Series>> seriesPtr;
	ScrollingBuffer<float> time;
};

#endif