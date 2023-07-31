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
		std::unique_ptr<ScrollingBuffer<double>> buffer;
		bool visible = true;
	};

	struct Marker
	{
		bool state = false;
		double value = 0.0f;
	};

	enum class type_E : uint8_t
	{
		CURVE = 0,
		BAR = 1,
		TABLE = 2,
	};

	Plot(std::string name);
	void setName(const std::string& newName);
	std::string getName() const;
	std::string& getNameVar();
	bool addSeries(Variable& var);
	std::shared_ptr<Plot::Series> getSeries(const std::string& name);
	std::map<std::string, std::shared_ptr<Plot::Series>>& getSeriesMap();
	ScrollingBuffer<double>& getTimeSeries();
	bool removeSeries(const std::string& name);
	bool removeAllVariables();
	std::vector<uint32_t> getVariableAddesses() const;
	std::vector<Variable::type> getVariableTypes() const;
	bool addPoint(const std::string& varName, double value);
	bool addTimePoint(double t);
	void erase();
	void setVisibility(bool state);
	bool getVisibility() const;
	bool& getVisibilityVar();

	bool getMarkerStateX0();
	void setMarkerStateX0(bool state);
	double getMarkerValueX0();
	void setMarkerValueX0(double value);

	bool getMarkerStateX1();
	void setMarkerStateX1(bool state);
	double getMarkerValueX1();
	void setMarkerValueX1(double value);

	void setType(type_E newType);
	type_E getType() const;

	void setIsHovered(bool isHovered);
	bool isHovered() const;

	displayFormat getSeriesDisplayFormat(const std::string& name) const;
	void setSeriesDisplayFormat(const std::string& name, displayFormat format);
	std::string getSeriesValueString(const std::string& name, double value);

   private:
	std::string name;
	std::map<std::string, std::shared_ptr<Series>> seriesMap;
	ScrollingBuffer<double> time;
	bool visibility = true;
	type_E type = type_E::CURVE;
	bool isHoveredOver = false;

	Marker mx0;
	Marker mx1;
};

#endif