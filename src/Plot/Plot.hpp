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
	enum class displayFormat
	{
		DEC = 0,
		HEX = 1,
		BIN = 2,
	};
	struct Series
	{
		Variable* var;
		displayFormat format = displayFormat::DEC;
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
	void setName(const std::string& newName);
	std::string getName() const;
	std::string& getNameVar();
	bool addSeries(Variable& var);
	std::shared_ptr<Plot::Series> getSeries(const std::string& name);
	std::map<std::string, std::shared_ptr<Plot::Series>>& getSeriesMap();
	ScrollingBuffer<float>& getTimeSeries();
	bool removeSeries(const std::string& name);
	bool removeAllVariables();
	std::vector<uint32_t> getVariableAddesses() const;
	std::vector<Variable::type> getVariableTypes() const;
	bool addPoint(const std::string& varName, const float value);
	bool addTimePoint(const float t);
	void erase();
	void setVisibility(bool state);
	bool getVisibility() const;
	bool& getVisibilityVar();

	void setType(const type_E newType);
	type_E getType() const;

	displayFormat getSeriesDisplayFormat(const std::string& name) const;
	void setSeriesDisplayFormat(const std::string& name, const displayFormat format);
	std::string getSeriesValueString(const std::string& name, const float value);

   private:
	std::string name;
	std::map<std::string, std::shared_ptr<Series>> seriesMap;
	ScrollingBuffer<float> time;
	bool visibility = true;
	type_E type = type_E::CURVE;
};

#endif