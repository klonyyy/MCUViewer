#ifndef _ITRACEDEVICE_HPP
#define _ITRACEDEVICE_HPP

#include <stdint.h>

#include <vector>

class ITraceDevice
{
   public:
	typedef struct
	{
		uint32_t debugProbe = 0;
		std::string serialNumber = "";
		std::string device = "";
		uint32_t speedkHz = 10000;

	} TraceProbeSettings;

	virtual ~ITraceDevice() = default;
	virtual bool startTrace(const TraceProbeSettings& probeSettings, uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset) = 0;
	virtual bool stopTrace() = 0;
	virtual int32_t readTraceBuffer(uint8_t* buffer, uint32_t size) = 0;

	virtual std::string getTargetName() = 0;
	virtual std::vector<std::string> getConnectedDevices() = 0;
};

#endif