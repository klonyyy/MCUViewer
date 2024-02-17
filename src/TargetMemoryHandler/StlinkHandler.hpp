#ifndef _STLINKHANDLER_HPP
#define _STLINKHANDLER_HPP

#include <string>
#include <vector>

#include "IDebugProbe.hpp"
#include "stlink.h"

class StlinkHandler : public IDebugProbe
{
   public:
	StlinkHandler();
	bool startAcqusition(const std::string& serialNumber, const std::string& device = "") override;
	bool stopAcqusition() override;
	bool isValid() const override;

	bool readMemory(uint32_t address, uint32_t* value) override;
	bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) override;

	std::string getLastErrorMsg() const override;
	std::vector<std::string> getConnectedDevices() override;
	bool requiresAlignedAccessOnRead() override
	{
		return true;
	}

   private:
	stlink_t* sl = nullptr;
	bool isRunning = false;
	std::string lastErrorMsg = "";
};

#endif