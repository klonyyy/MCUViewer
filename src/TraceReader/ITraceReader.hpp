#ifndef _ITRACEREADER_HPP
#define _ITRACEREADER_HPP

#include <string>
#include <vector>

#include "ScrollingBuffer.hpp"

class ITraceReader
{
   public:
	virtual ~ITraceReader() = default;
	virtual bool startAcqusition() = 0;
	virtual bool stopAcqusition() = 0;
	virtual bool isValid() const = 0;

	virtual bool readTrace(double& timestamp, std::array<bool, 10>& trace) = 0;

	virtual std::string getLastErrorMsg() const = 0;
};

#endif