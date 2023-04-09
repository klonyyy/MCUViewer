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
		Variable* var;
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
	bool addSeries(Variable& var);
	std::shared_ptr<Plot::Series> getSeries(std::string name);
	std::map<std::string, std::shared_ptr<Plot::Series>>& getSeriesMap();
	ScrollingBuffer<float>& getTimeSeries();
	bool removeSeries(std::string name);
	bool removeAllVariables();
	std::vector<uint32_t> getVariableAddesses();
	std::vector<Variable::type> getVariableTypes();
	bool addPoint(std::string name, float value);
	bool addTimePoint(float t);
	void erase();
	void setVisibility(bool state);
	bool getVisibility();
	bool& getVisibilityVar();

	void setType(type_E newType);
	type_E getType();

   private:
	std::string name;
	std::map<std::string, std::shared_ptr<Series>> seriesMap;
	ScrollingBuffer<float> time;
	bool visibility = true;
	type_E type = type_E::CURVE;
};

#endif