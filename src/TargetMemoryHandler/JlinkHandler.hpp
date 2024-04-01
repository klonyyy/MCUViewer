#ifndef _JLINKHANDLER_HPP
#define _JLINKHANDLER_HPP

#include <string>
#include <vector>

#include "IDebugProbe.hpp"
#include "jlink.h"
#include "spdlog/spdlog.h"

class JlinkHandler : public IDebugProbe
{
   public:
	JlinkHandler(spdlog::logger* logger);
	bool startAcqusition(const std::string& serialNumber, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, Mode mode = Mode::NORMAL, const std::string& device = "") override;
	bool stopAcqusition() override;
	bool isValid() const override;

	bool initRead() const override;
	bool readMemory(uint32_t address, uint32_t* value) override;
	bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) override;

	std::string getLastErrorMsg() const override;
	std::vector<std::string> getConnectedDevices() override;

	bool requiresAlignedAccessOnRead() override
	{
		return false;
	}

   private:
	static constexpr size_t maxDevices = 10;
	std::unordered_map<uint32_t, uint32_t> addressValueMap;
	bool isRunning = false;

	std::string lastErrorMsg = "";
	spdlog::logger* logger;
};

#endif