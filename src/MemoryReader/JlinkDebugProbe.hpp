#ifndef _JlinkDebugProbe_HPP
#define _JlinkDebugProbe_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "IDebugProbe.hpp"
#include "jlink.h"
#include "spdlog/spdlog.h"

class JlinkDebugProbe : public IDebugProbe
{
   public:
	JlinkDebugProbe(spdlog::logger* logger);
	bool startAcqusition(const DebugProbeSettings& probeSettings, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency) override;
	bool stopAcqusition() override;
	bool isValid() const override;
	std::string getTargetName() override;

	std::optional<IDebugProbe::varEntryType> readSingleEntry() override;

	bool readMemory(uint32_t address, uint32_t* value) override;
	bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) override;

	std::string getLastErrorMsg() const override;
	std::vector<std::string> getConnectedDevices() override;

	bool requiresAlignedAccessOnRead() override { return false; }

   private:
	static constexpr size_t maxDevices = 10;
	static constexpr size_t maxVariables = 100;
	static constexpr size_t fifoSize = 2000;
	static constexpr uint32_t maxSpeedkHz = 50000;
	static constexpr double timestampResolution = 1e-6;

	JLINK_HSS_MEM_BLOCK_DESC variableDesc[maxVariables]{};
	size_t trackedVarsCount = 0;
	size_t trackedVarsTotalSize = 0;

	std::unordered_map<uint32_t, uint8_t> addressSizeMap;
	RingBuffer<varEntryType, fifoSize> varTable;

	bool isRunning = false;
	std::string lastErrorMsg = "";
	spdlog::logger* logger;
};

#endif