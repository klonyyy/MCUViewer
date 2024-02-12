#ifndef _JLINKHANDLER_HPP
#define _JLINKHANDLER_HPP

#include <windows.h>

#include <string>
#include <vector>

#include "ITargetMemoryHandler.hpp"

class JlinkHandler : public ITargetMemoryHandler
{
	struct JlinkFunctions
	{
		static constexpr size_t maxDevices = 10;
		typedef struct JLINKARM_EMU_CONNECT_INFO
		{
			uint32_t serialNumber;
			uint32_t connection;
			uint32_t USBAddr;
			uint8_t aIPAddr[16];
			int32_t time;
			uint64_t TimeUs;
			uint32_t HWVersion;
			uint8_t abMACAdr[6];
			uint8_t acProduct[32];
			uint8_t acNickName[32];
			uint8_t acFWString[112];
			uint8_t isDHCPAssignedIP;
			uint8_t isDHCPAssignedIPIsValid;
			uint8_t numIPConnections;
			uint8_t numIPConnectionsIsValid;
			uint8_t aPadding[34];

		} JLINKARM_EMU_CONNECT_INFO;

		int32_t (*jlinkGetList)(int hostIFs, JLINKARM_EMU_CONNECT_INFO* paConnectInfo, int MaxInfos);

		int (*jlink_select_by_usb)(int);
		int (*jlink_lock)(int);
		int (*jlink_open)(void* log, void* errHandler);
		void (*jlink_close)();
		bool (*jlink_is_open)();
		bool (*jlink_read_mem)(uint32_t addr, int32_t size, uint8_t* buf, void* access);
		bool (*jlink_write_mem)(uint32_t addr, int32_t size, uint8_t* buf, void* access);
	};

   public:
	JlinkHandler();
	~JlinkHandler();
	bool startAcqusition() override;
	bool stopAcqusition() override;
	bool isValid() const override;

	bool readMemory(uint32_t address, uint32_t* value) override;
	bool writeMemory(uint32_t address, uint8_t* buf, uint32_t len) override;

	std::string getLastErrorMsg() const override;
	std::vector<uint32_t> getConnectedDevices() override;

	bool requiresAlignedAccessOnRead() override
	{
		return false;
	}

   private:
	HMODULE so_handle;
	JlinkFunctions jlinkFunctions;

	bool isRunning = false;
	std::string lastErrorMsg = "";
};

#endif