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
		double Lmin;
		double Lmax;
		double Hmin;
		double Hmax;
		double fmin;
		double fmax;
	};

	static void calculateResults(Plot::Series* ser, ScrollingBuffer<double>* time, double start, double end, DigitalResults& results)
	{
		auto data = ser->buffer->getLinearData(time->getIndexFromvalue(start) + 1, time->getIndexFromvalue(end) + 1);
		std::vector<double> timeData = time->getLinearData(time->getIndexFromvalue(start) + 1, time->getIndexFromvalue(end) + 1);

		std::vector<double> Lvec, Hvec;

		convertDigitalSeriesToVectors(timeData, data, Lvec, Hvec);

		results.Lmin = findmin(Lvec);
		results.Lmax = findmax(Lvec);
		results.Hmin = findmin(Hvec);
		results.Hmax = findmax(Hvec);

		auto shorter = Lvec.size() < Hvec.size() ? Lvec.size() : Hvec.size();

		std::vector<double> T;
		std::vector<double> f;

		for (size_t i = 0; i < shorter; i++)
		{
			auto sum = Lvec[i] + Hvec[i];
			T.push_back(sum);
			f.push_back(1 / sum);
		}

		results.fmin = findmin(f);
		results.fmax = findmax(f);
	}

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

	static void convertDigitalSeriesToVectors(std::vector<double> time, std::vector<double> data, std::vector<double>& Lvec, std::vector<double>& Hvec)
	{
		double lastState = data[0];
		double timeStart = time[0];

		size_t i = 0;
		for (auto& state : data)
		{
			if (lastState != state)
			{
				if (state > 0.0)
					Lvec.push_back(time[i] - timeStart);
				else
					Hvec.push_back(time[i] - timeStart);

				timeStart = time[i];
				lastState = state;
			}
			i++;
		}
	}
};

#endif