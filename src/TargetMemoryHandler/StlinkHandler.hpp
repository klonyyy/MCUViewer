#ifndef _STLINKHANDLER_HPP
#define _STLINKHANDLER_HPP

#include <string>
#include <vector>

#include "ITargetMemoryHandler.hpp"
#include "stlink.h"

class StlinkHandler : public ITargetMemoryHandler
{
   public:
	StlinkHandler();
	bool startAcqusition() override;
	bool stopAcqusition() override;
	bool isValid() const override;

	bool readMemory(uint32_t address, uint32_t* value) override;
	bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) override;

	std::string getLastErrorMsg() const override;
	std::vector<uint32_t> getConnectedDevices() override;
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