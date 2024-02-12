#ifndef _IVARIABLEREADER_HPP
#define _IVARIABLEREADER_HPP

#include <cstdint>
#include <string>
#include <vector>

class ITargetMemoryHandler
{
   public:
	virtual ~ITargetMemoryHandler() = default;
	virtual bool startAcqusition() = 0;
	virtual bool stopAcqusition() = 0;
	virtual bool isValid() const = 0;

	virtual bool readMemory(uint32_t address, uint32_t* value) = 0;
	virtual bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) = 0;

	virtual std::string getLastErrorMsg() const = 0;

	virtual std::vector<uint32_t> getConnectedDevices() = 0;
	virtual bool requiresAlignedAccessOnRead() = 0;
};

#endif