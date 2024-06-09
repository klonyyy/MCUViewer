#ifndef MOVINGAVG_HPP
#define MOVINGAVG_HPP

#include <cstdlib>

class MovingAverage
{
   public:
	MovingAverage(const size_t samples_);
	double filter(double sampleIn);

   private:
	static constexpr size_t maxSamples = 1000;
	size_t samples = 0;
	double buffer[maxSamples] = {0};
	size_t bufferIter = 0;
	double bufferSum = 0;
};

#endif