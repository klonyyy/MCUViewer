#ifndef _STLINKTRACEDEVICE_HPP
#define _STLINKTRACEDEVICE_HPP

#include "ITraceDevice.hpp"
#include "spdlog/spdlog.h"
#include "stlink.h"

class StlinkTraceDevice : public ITraceDevice
{
   public:
	StlinkTraceDevice(std::shared_ptr<spdlog::logger> logger);
	bool startTrace(uint32_t coreFrequency, uint32_t traceFrequency) override;
	bool stopTrace() override;
	uint32_t readTraceBuffer(uint8_t* buffer, uint32_t size) override;

   private:
	stlink_t* sl = nullptr;
	std::shared_ptr<spdlog::logger> logger;
};
#endif