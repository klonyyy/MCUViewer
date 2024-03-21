#ifndef JLINKDYNAMICLIBRARYLOADER_HPP
#define JLINKDYNAMICLIBRARYLOADER_HPP

#include <cstdint>

#include "../commons.hpp"

#pragma GCC optimize("-fno-strict-aliasing")
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

	int32_t (*jlinkGetList)(int32_t hostIFs, JLINKARM_EMU_CONNECT_INFO* paConnectInfo, int32_t MaxInfos);
	int32_t (*jlinkSelectByUsb)(uint32_t serialNo);
	const char* (*jlinkOpen)(void* log, void* errHandler);
	void (*jlinkClose)();
	bool (*jlinkIsOpen)();
	int32_t (*jlinkReadMem)(uint32_t addr, uint32_t size, void* buf, uint32_t access);
	int32_t (*jlinkWriteMem)(uint32_t addr, uint32_t size, const void* buf, uint32_t access);
	int32_t (*jlinkExecCommand)(const char* sIn, char* sError, int32_t bufferSize);
	int32_t (*jlinkTifSelect)(int32_t interface);
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

		handle = LoadLibrary(primaryPath);

		if (handle == nullptr)
			handle = LoadLibrary(secondaryPath);

		if (handle == nullptr)
			return false;

		castLambda(jlinkFunctions.jlinkOpen, "JLINKARM_OpenEx");
		castLambda(jlinkFunctions.jlinkClose, "JLINKARM_Close");
		castLambda(jlinkFunctions.jlinkIsOpen, "JLINKARM_IsOpen");
		castLambda(jlinkFunctions.jlinkReadMem, "JLINKARM_ReadMemEx");
		castLambda(jlinkFunctions.jlinkWriteMem, "JLINKARM_WriteMemEx");
		castLambda(jlinkFunctions.jlinkGetList, "JLINKARM_EMU_GetList");
		castLambda(jlinkFunctions.jlinkSelectByUsb, "JLINKARM_EMU_SelectByUSBSN");
		castLambda(jlinkFunctions.jlinkExecCommand, "JLINKARM_ExecCommand");
		castLambda(jlinkFunctions.jlinkTifSelect, "JLINKARM_TIF_Select");

		return true;
	}

   private:
	HMODULE handle;
	const char* primaryPath = "JLink_x64.dll";
	const char* secondaryPath = "C:/Program Files/SEGGER/JLink/JLink_x64.dll";
};

using DynamicLibraryLoader = WindowsDllLoader;

#endif

#if defined(__APPLE__) || defined(_UNIX)

#include <dlfcn.h>

class UnixSoLoader
{
   public:
	~UnixSoLoader()
	{
		dlclose(handle);
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

		handle = dlopen(primaryPath, RTLD_NOW);

		if (handle == nullptr)
			handle = dlopen(secondaryPath, RTLD_NOW);

		if (handle == nullptr)
			return false;

		castLambda(jlinkFunctions.jlinkOpen, "JLINKARM_OpenEx");
		castLambda(jlinkFunctions.jlinkClose, "JLINKARM_Close");
		castLambda(jlinkFunctions.jlinkIsOpen, "JLINKARM_IsOpen");
		castLambda(jlinkFunctions.jlinkReadMem, "JLINKARM_ReadMemEx");
		castLambda(jlinkFunctions.jlinkWriteMem, "JLINKARM_WriteMemEx");
		castLambda(jlinkFunctions.jlinkGetList, "JLINKARM_EMU_GetList");
		castLambda(jlinkFunctions.jlinkSelectByUsb, "JLINKARM_EMU_SelectByUSBSN");
		castLambda(jlinkFunctions.jlinkExecCommand, "JLINKARM_ExecCommand");

		return true;
	}

   private:
	void* handle = nullptr;

	const char* primaryPath = "libjlinkarm.so";
	const char* secondaryPath = "/opt/SEGGER/JLink/libjlinkarm.so";
};
using DynamicLibraryLoader = UnixSoLoader;
#endif

#endif