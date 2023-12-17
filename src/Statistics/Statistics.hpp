#ifndef STATISTICS_HPP_
#define STATISTICS_HPP_

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "Plot.hpp"
#include "ScrollingBuffer.hpp"

#pragma once
#ifndef TEST_FRIENDS_STATISTICS
#define TEST_FRIENDS_STATISTICS
#endif

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
			f.push_back(1.0 / sum);
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
	TEST_FRIENDS_STATISTICS

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
		/* find the first and last signal change */
		size_t start = 0;
		size_t end = time.size() - 1;
		size_t i = 1;

		while (data[i] == data[0] && i < data.size())
			i++;
		start = i;

		i = end - 1;

		while (data[i] == data[end] && i > 0)
			i--;
		end = i;

		double lastState = data[start];
		double timeStart = time[start];

		for (i = start; i <= end; i++)
		{
			if (lastState != data[i] && timeStart < time[i])
			{
				if (data[i] > 0.0)
					Lvec.push_back(time[i] - timeStart);
				else
					Hvec.push_back(time[i] - timeStart);

				timeStart = time[i];
				lastState = data[i];
			}
		}
	}
};

#endif