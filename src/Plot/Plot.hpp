#ifndef _PLOT_HPP
#define _PLOT_HPP

#include <functional>
#include <map>
#include <memory>
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
		Variable::Color* color;
		std::string* seriesName;
		std::unique_ptr<ScrollingBuffer<float>> buffer;
	};

	enum class type_E : uint8_t
	{
		CURVE = 0,
		BAR = 1,
		TABLE = 2,
	};

	Plot(std::string name);
	~Plot();
	void setName(std::string newName);
	std::string getName() const;
	std::string& getNameVar();
	bool addSeries(std::string* name, uint32_t address, Variable::Color& color);
	bool addSeries(Variable& var);
	std::shared_ptr<Plot::Series> getSeries(uint32_t address);
	std::map<uint32_t, std::shared_ptr<Plot::Series>>& getSeriesMap();
	ScrollingBuffer<float>& getTimeSeries();
	bool removeVariable(uint32_t address);
	bool removeAllVariables();
	std::vector<uint32_t> getVariableAddesses();
	std::vector<Variable::type> getVariableTypes();
	bool addPoint(uint32_t address, float value);
	bool addTimePoint(float t);
	void erase();
	void setVisibility(bool state);
	bool getVisibility();
	bool& getVisibilityVar();

	void setType(type_E newType);
	type_E getType();

   private:
	std::string name;
	std::map<uint32_t, std::shared_ptr<Series>> seriesMap;
	ScrollingBuffer<float> time;
	bool visibility = true;
	type_E type = type_E::CURVE;
};

#endif