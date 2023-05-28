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
		bool visible = true;
	};

	typedef struct Marker
	{
		bool state = false;
		float value = 0.0f;
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
	bool addPoint(const std::string& varName, float value);
	bool addTimePoint(float t);
	void erase();
	void setVisibility(bool state);
	bool getVisibility() const;
	bool& getVisibilityVar();

	bool getMarkerStateX0();
	void setMarkerStateX0(bool state);
	float getMarkerValueX0();
	void setMarkerValueX0(float value);

	bool getMarkerStateX1();
	void setMarkerStateX1(bool state);
	float getMarkerValueX1();
	void setMarkerValueX1(float value);

	void setType(type_E newType);
	type_E getType() const;

	displayFormat getSeriesDisplayFormat(const std::string& name) const;
	void setSeriesDisplayFormat(const std::string& name, displayFormat format);
	std::string getSeriesValueString(const std::string& name, float value);

   private:
	std::string name;
	std::map<std::string, std::shared_ptr<Series>> seriesMap;
	ScrollingBuffer<float> time;
	bool visibility = true;
	type_E type = type_E::CURVE;

	Marker mx0;
	Marker mx1;
};

#endif