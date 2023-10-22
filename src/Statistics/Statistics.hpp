#ifndef STATISTICS_HPP_
#define STATISTICS_HPP_

#include <algorithm>
#include <vector>

#include "Plot.hpp"
#include "ScrollingBuffer.hpp"

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

	static void calculateResults(Plot::Series* ser, ScrollingBuffer<double>* time, double start, double end, AnalogResults& results)
	{
		/* + 1 is to account for the way sample is "held" for the entire duration of sample period */
		auto data = ser->buffer->getLinearData(time->getIndexFromvalue(start) + 1, time->getIndexFromvalue(end) + 1);
		results.min = findmin(data);
		results.max = findmax(data);
		results.mean = mean(data);
		results.stddev = stddev(data);
	}

   private:
	static double findmin(std::vector<double> data)
	{
		if (data.empty())
			return 0.0;
		return *std::min_element(data.begin(), data.end());
	}

	static double findmax(std::vector<double> data)
	{
		if (data.empty())
			return 0.0;
		return *std::max_element(data.begin(), data.end());
	}

	static double mean(std::vector<double> data)
	{
		if (data.empty())
			return 0.0;
		return std::accumulate(data.begin(), data.end(), 0.0) / static_cast<double>(data.size());
	}

	static double stddev(std::vector<double> data)
	{
		if (data.empty())
			return 0.0;

		double m = mean(data);

		double variance = 0.0;
		for (const double& value : data)
			variance += (value - m) * (value - m);

		variance /= static_cast<double>(data.size());

		return std::sqrt(variance);
	}
};

#endif