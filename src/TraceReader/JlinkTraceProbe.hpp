#ifndef _JLINKTRACEPROBE_HPP
#define _JLINKTRACEPROBE_HPP

#include <memory>

#include "ITraceProbe.hpp"
#include "jlink.h"
#include "spdlog/spdlog.h"

class JlinkTraceProbe : public ITraceProbe
{
   public:
	explicit JlinkTraceProbe(spdlog::logger* logger);
	bool startTrace(const TraceProbeSettings& probeSettings, uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset) override;
	bool stopTrace() override;
	int32_t readTraceBuffer(uint8_t* buffer, uint32_t size) override;

	std::string getTargetName() override;
	std::vector<std::string> getConnectedDevices() override;

   private:
	static constexpr uint32_t maxSpeedkHz = 50000;
	static constexpr size_t maxDevices = 10;
	spdlog::logger* logger;
};
#endif