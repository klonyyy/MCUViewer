#ifndef _PLOT_HPP
#define _PLOT_HPP

#include <functional>
#include <map>
#include <memory>
#include <mutex>
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

	enum class Type : uint8_t
	{
		CURVE = 0,
		BAR = 1,
		TABLE = 2,
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

	explicit Plot(std::string name);
	void setName(const std::string& newName);
	std::string getName() const;
	std::string& getNameVar();
	void setAlias(const std::string& newAlias);
	std::string getAlias() const;
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

	void setType(Type newType);
	Type getType() const;

	/* TODO: Domain and TraceVarType should be in a derived class only */
	void setDomain(Domain newDomain);
	Domain getDomain() const;

	void setTraceVarType(TraceVarType newTraceVarType);
	TraceVarType getTraceVarType() const;

	void setIsHovered(bool isHovered);
	bool isHovered() const;

	displayFormat getSeriesDisplayFormat(const std::string& name) const;
	void setSeriesDisplayFormat(const std::string& name, displayFormat format);
	std::string getSeriesValueString(const std::string& name, double value);

   private:
	std::string name;
	std::string alias;
	std::map<std::string, std::shared_ptr<Series>> seriesMap;
	ScrollingBuffer<double> time;
	bool visibility = true;
	Type type = Type::CURVE;
	Domain domain = Domain::ANALOG;
	TraceVarType traceVarType = TraceVarType::F32;
	bool isHoveredOver = false;

	Marker mx0;
	Marker mx1;
};

#endif