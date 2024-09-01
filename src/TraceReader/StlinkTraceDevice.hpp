#ifndef _STLINKTRACEDEVICE_HPP
#define _STLINKTRACEDEVICE_HPP

#include <memory>

#include "ITraceDevice.hpp"
#include "spdlog/spdlog.h"
#include "stlink.h"

class StlinkTraceDevice : public ITraceDevice
{
   public:
	explicit StlinkTraceDevice(spdlog::logger* logger);
	bool startTrace(const TraceProbeSettings& probeSettings, uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset) override;
	bool stopTrace() override;
	int32_t readTraceBuffer(uint8_t* buffer, uint32_t size) override;

	std::string getTargetName() override { return std::string(); }
	std::vector<std::string> getConnectedDevices() override;

   private:
	stlink_t* sl = nullptr;
	spdlog::logger* logger;
};
#endif