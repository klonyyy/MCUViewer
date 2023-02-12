#ifndef _PLOT_HPP
#define _PLOT_HPP

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
		std::string seriesName;
		ScrollingBuffer<float>* buffer;
	};

	Plot(std::string name);
	~Plot();
	bool addSeries(std::string name, uint32_t address);
	bool addSeries(Variable var);
	bool removeVariable(uint32_t address);
	bool removeAllVariables();
	std::vector<uint32_t> getVariableAddesses();
	std::vector<Variable::type> getVariableTypes();
	bool addPoint(float t, uint32_t address, float value);
	bool addTimePoint(float t);
	void draw();
	void erase();

   private:
	std::mutex mtx;
	std::string name;
	std::map<uint32_t, Series*> seriesPtr;
	ScrollingBuffer<float> time;
};

#endif