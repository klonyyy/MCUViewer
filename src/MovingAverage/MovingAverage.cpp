
#include "MovingAverage.hpp"

MovingAverage::MovingAverage(size_t samples_) : samples(samples_ > maxSamples ? maxSamples : samples_)
{
}

double MovingAverage::filter(double sampleIn)
{
	bufferSum -= buffer[bufferIter];  // Remove last entry from sum
	buffer[bufferIter] = sampleIn;	  // Add new entry in place of the last one
	bufferSum += buffer[bufferIter];  // Add new entry to sum
	bufferIter++;					  // Increase iterator
	if (bufferIter >= samples)
		bufferIter = 0;

	return bufferSum / samples;
}