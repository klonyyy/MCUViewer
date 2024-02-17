#ifndef JLINKDYNAMICLIBRARYLOADER_HPP
#define JLINKDYNAMICLIBRARYLOADER_HPP

#include <cstdint>

#include "../commons.hpp"

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
	bool (*jlinkSelectByUsb)(int32_t id);

	int (*jlink_select_by_usb)(int);
	int (*jlink_lock)(int);
	const char* (*jlink_open)(void* log, void* errHandler);
	void (*jlink_close)();
	bool (*jlink_is_open)();
	bool (*jlink_read_mem)(uint32_t addr, int32_t size, uint8_t* buf, void* access);
	bool (*jlink_write_mem)(uint32_t addr, int32_t size, uint8_t* buf, void* access);
};

#ifdef _WIN32
#include <windows.h>

class WindowsDllLoader
{
   public:
	~WindowsDllLoader()
	{
		FreeLibrary(handle);
	}

	bool doLoad(JlinkFunctions& jlinkFunctions)
	{
		auto castLambda = [&](auto& func, const char* name)
		{
			auto proc = GetProcAddress(handle, name);
			if (proc)
			{
				func = reinterpret_cast<decltype(func)>(proc);
			}
		};

		handle = LoadLibrary("C:/Program Files/SEGGER/JLink/JLink_x64.dll");

		if (handle == nullptr)
			return false;

		castLambda(jlinkFunctions.jlink_lock, "JLock");
		castLambda(jlinkFunctions.jlink_open, "JLINKARM_OpenEx");
		castLambda(jlinkFunctions.jlink_close, "JLINKARM_Close");
		castLambda(jlinkFunctions.jlink_is_open, "JLINKARM_IsOpen");
		castLambda(jlinkFunctions.jlink_read_mem, "JLINKARM_ReadMemEx");
		castLambda(jlinkFunctions.jlink_write_mem, "JLINKARM_WriteMemEx");
		castLambda(jlinkFunctions.jlinkGetList, "JLINKARM_EMU_GetList");
		castLambda(jlinkFunctions.jlinkSelectByUsb, "JLINKARM_EMU_SelectByUSBSN");

		return true;
	}

   private:
	HMODULE handle;
};

using DynamicLibraryLoader = WindowsDllLoader;

#endif

#ifdef _UNIX

#include <dlfcn.h>

class UnixSoLoader
{
   public:
	~UnixSoLoader()
	{
	}

	bool doLoad(JlinkFunctions& jlinkFunctions)
	{
		auto castLambda = [&](auto& func, const char* name)
		{
			auto proc = dlsym(handle, name);
			if (proc)
			{
				func = reinterpret_cast<decltype(func)>(proc);
			}
		};

		handle = dlopen("/opt/SEGGER/JLink/libjlinkarm.so", RTLD_NOW);

		if (handle == nullptr)
			return false;

		castLambda(jlinkFunctions.jlink_lock, "JLock");
		castLambda(jlinkFunctions.jlink_open, "JLINKARM_OpenEx");
		castLambda(jlinkFunctions.jlink_close, "JLINKARM_Close");
		castLambda(jlinkFunctions.jlink_is_open, "JLINKARM_IsOpen");
		castLambda(jlinkFunctions.jlink_read_mem, "JLINKARM_ReadMemEx");
		castLambda(jlinkFunctions.jlink_write_mem, "JLINKARM_WriteMemEx");
		castLambda(jlinkFunctions.jlinkGetList, "JLINKARM_EMU_GetList");
		castLambda(jlinkFunctions.jlinkSelectByUsb, "JLINKARM_EMU_SelectByUSBSN");

		return true;
	}

   private:
};
using DynamicLibraryLoader = UnixSoLoader;
#endif

#endif