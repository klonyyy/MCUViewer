#ifndef _STLINKHANDLER_HPP
#define _STLINKHANDLER_HPP

#include <string>
#include <vector>

#include "IDebugProbe.hpp"
#include "spdlog/spdlog.h"
#include "stlink.h"

class StlinkHandler : public IDebugProbe
{
   public:
	StlinkHandler(spdlog::logger* logger);
	bool startAcqusition(const std::string& serialNumber, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency, Mode mode = Mode::NORMAL, const std::string& device = "") override;
	bool stopAcqusition() override;
	bool isValid() const override;

	std::optional<IDebugProbe::varEntryType> readSingleEntry() override;
	bool readMemory(uint32_t address, uint32_t* value) override;
	bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) override;

	std::string getLastErrorMsg() const override;
	std::vector<std::string> getConnectedDevices() override;
	bool requiresAlignedAccessOnRead() override
	{
		return true;
	}

   private:
	int IsChipHalted(stlink_t* sl);
	int IsChipSleeping(stlink_t* sl);
	bool IsWaitForWakeSuccess(stlink_t* sl);
	bool IsWaitForResumeSuccess(stlink_t* sl);
	stlink_t* sl = nullptr;
	bool isRunning = false;
	std::string lastErrorMsg = "";
	spdlog::logger* logger;
};

#endif