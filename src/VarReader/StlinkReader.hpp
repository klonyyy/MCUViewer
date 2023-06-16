#ifndef _STLINKREADER_HPP
#define _STLINKREADER_HPP

#include "IVariableReader.hpp"
#include "stlink.h"

class StlinkReader : public IVariableReader
{
   public:
	bool startAcqusition() override;
	bool stopAcqusition() override;
	bool isValid() const override;

	bool readMemory(uint32_t address, uint32_t* value) override;
	bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) override;

	std::string getLastErrorMsg() const override;

   private:
	stlink_t* sl = nullptr;
	bool isRunning = false;
	std::string lastErrorMsg = "";
};

#endif