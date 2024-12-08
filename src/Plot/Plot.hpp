#ifndef _PLOT_HPP
#define _PLOT_HPP

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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
		Variable* var = nullptr;
		displayFormat format = displayFormat::DEC;
		std::unique_ptr<ScrollingBuffer<double>> buffer;
		bool visible = true;

		void addPointFromVar() { buffer->addPoint(var->getValue()); }
	};

	enum class Type : uint8_t
	{
		CURVE = 0,
		BAR = 1,
		TABLE = 2,
		XY = 3
	};

	enum class Domain : uint8_t
	{
		ANALOG = 0,
		DIGITAL = 1,
	};

	enum class TraceVarType : uint8_t
	{
		U8 = 0,
		I8 = 1,
		U16 = 2,
		I16 = 3,
		U32 = 4,
		I32 = 5,
		F32 = 6
	};

	class DragRect
	{
	   public:
		bool getState() const { return state; }
		void setState(bool newState) { state = newState; }

		double getValueX0() const { return std::min(rect.x0, rect.x1); }
		double getValueX1() const { return std::max(rect.x0, rect.x1); }

		void setValueX0(double newX0) { rect.x0 = newX0; }
		void setValueX1(double newX1) { rect.x1 = newX1; }

	   private:
		bool state = false;

		struct Rect
		{
			double x0;
			double x1;
			double y0;
			double y1;
		} rect{};
	};

	class Marker
	{
	   public:
		Marker() : state(false), value(0.0) {}

		bool getState() const { return state; }
		void setState(bool newState) { state = newState; }

		double getValue() const { return value; }
		void setValue(double newValue) { value = newValue; }

	   private:
		bool state;
		double value;
	};

	Marker markerX0{};
	Marker markerX1{};
	Marker trigger{};

	DragRect stats{};

	explicit Plot(const std::string& name);
	void setName(const std::string& newName);
	std::string getName() const;
	std::string& getNameVar();
	void setAlias(const std::string& newAlias);
	std::string getAlias() const;
	bool addSeries(Variable* var);
	std::shared_ptr<Plot::Series> getSeries(const std::string& name);
	std::map<std::string, std::shared_ptr<Plot::Series>>& getSeriesMap();
	ScrollingBuffer<double>* getXAxisSeries();
	bool removeSeries(const std::string& name);
	bool removeAllVariables();
	void renameSeries(const std::string& oldName, const std::string newName);
	std::vector<uint32_t> getVariableAddesses() const;
	std::vector<Variable::Type> getVariableTypes() const;
	bool addPoint(const std::string& varName, double value);
	void updateSeries();
	bool addTimePoint(double t);
	void erase();
	void setVisibility(bool state);
	bool getVisibility() const;
	bool& getVisibilityVar();

	void setType(Type newType);
	Type getType() const;

	/* TODO: Domain and TraceVarType should be in a derived class only */
	void setDomain(Domain newDomain);
	Domain getDomain() const;

	void setTraceVarType(TraceVarType newTraceVarType);
	TraceVarType getTraceVarType() const;

	void setIsHovered(bool isHovered);
	bool isHovered() const;

	Variable* getXAxisVariable();
	void setXAxisVariable(Variable* var);

	displayFormat getSeriesDisplayFormat(const std::string& name) const;
	void setSeriesDisplayFormat(const std::string& name, displayFormat format);
	std::string getSeriesValueString(const std::string& name, double value);

	int32_t statisticsSeries = 0;

   private:
	std::string name;
	std::string alias;
	std::map<std::string, std::shared_ptr<Series>> seriesMap;
	ScrollingBuffer<double> time;
	Series xAxisSeries;
	bool visibility = true;
	Type type = Type::CURVE;
	Domain domain = Domain::ANALOG;
	TraceVarType traceVarType = TraceVarType::F32;
	bool isHoveredOver = false;

	Marker mx0;
	Marker mx1;
};

#endif