#ifndef _IVARIABLEREADER_HPP
#define _IVARIABLEREADER_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class IDebugProbe
{
   public:
	enum Mode
	{
		NORMAL,
		HSS,
	};

	virtual ~IDebugProbe() = default;
	virtual bool startAcqusition(const std::string& serialNumber, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, Mode mode = Mode::NORMAL, const std::string& device = "") = 0;
	virtual bool stopAcqusition() = 0;
	virtual bool isValid() const = 0;

	virtual bool initRead() const = 0;
	virtual bool readMemory(uint32_t address, uint32_t* value) = 0;
	virtual bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) = 0;

	virtual std::string getLastErrorMsg() const = 0;

	virtual std::vector<std::string> getConnectedDevices() = 0;
	virtual bool requiresAlignedAccessOnRead() = 0;
};

#endif