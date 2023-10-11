#ifndef STATISTICS_HPP_
#define STATISTICS_HPP_

#include "Plot.hpp"

class Statistics
{
   public:
	struct AnalogResults
	{
		double min;
		double max;
		double mean;
		double stddev;
	};

	struct DigitalResults
	{
		double min;
		double max;
		double mean;
		double stddev;
	};

	void calculateResults(Plot::Series* ser, Plot::Series* time, double start, double end, AnalogResults& results)
	{
		auto data = ser->getLinearData(time.getIndexFromvalue(start), time.getIndexFromvalue(end));
        
	}

   private:
	double findmin()
	{
	}

	double findmax()
	{
	}

	double mean()
	{
	}

	double stddev()
	{
	}
}

#endif