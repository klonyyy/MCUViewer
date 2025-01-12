#ifndef _ITRACEPROBE_HPP
#define _ITRACEPROBE_HPP

#include <stdint.h>
#include <string>
#include <vector>

class ITraceProbe
{
   public:
	struct TraceProbeSettings
	{
		uint32_t debugProbe = 0;
		std::string serialNumber = "";
		std::string device = "";
		uint32_t speedkHz = 10000;

	};

	virtual ~ITraceProbe() = default;
	virtual bool startTrace(const TraceProbeSettings& probeSettings, uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset) = 0;
	virtual bool stopTrace() = 0;
	virtual int32_t readTraceBuffer(uint8_t* buffer, uint32_t size) = 0;

	virtual std::string getTargetName() = 0;
	virtual std::vector<std::string> getConnectedDevices() = 0;
};

#endif