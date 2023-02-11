#ifndef _PLOT_HPP
#define _PLOT_HPP

#include <map>
#include <thread>

#include "ScrollingBuffer.hpp"

class Plot
{
   public:
	struct Series
	{
		std::string seriesName;
		ScrollingBuffer<float>* buffer;
	};

	Plot();
	~Plot();
	bool addSeries(std::string name, uint32_t address);
	bool removeVariable(uint32_t address);
	bool removeAllVariables();
	std::vector<uint32_t> getVariableAddesses();
	bool addPoint(float t, uint32_t address, float value);
	void draw();

   private:
	std::map<uint32_t, Series*> seriesPtr;
	ScrollingBuffer<float> time;
};

#endif