
/*********************************************************************
 *                    SEGGER Microcontroller GmbH                     *
 *                        The Embedded Experts                        *
 **********************************************************************
 *                                                                    *
 *            (c) 2024 SEGGER Microcontroller GmbH                    *
 *                                                                    *
 *       www.segger.com     Support: support@segger.com               *
 *                                                                    *
 **********************************************************************
 *                                                                    *
 *             SEGGER JLinkARMDLL.h and JLINKARM_Const.h              *
 *                                                                    *
 **********************************************************************
 *                                                                    *
 *                  Licensed to Piotr Wasilewski                      *
 *                   for the utility STMViewer                        *
 *                                                                    *
 **********************************************************************
 *                                                                    *
 * All rights reserved.                                               *
 *                                                                    *
 * The JLinkARMDLL.h und JLINKARM_Const.h or parts thereof is SEGGER  *
 * IP and protected by copyright laws.                                *
 * SEGGER strongly recommends to not make any changes                 *
 * to or modify the source code of this software in order to stay     *
 * compatible with J-Link.                                            *
 *                                                                    *
 * Redistribution and use in source and binary forms                  *
 * without modification is permitted provided that the following      *
 * condition is met:                                                  *
 *                                                                    *
 * Redistributions of source code or binary must retain the above     *
 * copyright notice, this condition and the following disclaimer.     *
 *                                                                    *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
 * DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
 * DAMAGE.                                                            *
 *                                                                    *
 **********************************************************************/

#ifndef JLINKARM_H	//  Avoid multiple inclusion
#define JLINKARM_H

#include "stdint.h"

#define JLINK_HSS_FLAG_TIMESTAMP_US (1uL << 0)

#define JLINKARM_TIF_JTAG 0
#define JLINKARM_TIF_SWD  1

#if defined(__cplusplus)
extern "C"
{  // Make sure we have C-declarations in C++ programs
#endif

	typedef void JLINKARM_LOG(const char* sErr);

	typedef struct
	{
		uint32_t SerialNumber;		   // This is the serial number reported in the discovery process, which is the "true serial number" for newer J-Links and 123456 for older J-Links.
		unsigned Connection;		   // Either JLINKARM_HOSTIF_USB = 1 or JLINKARM_HOSTIF_IP = 2
		uint32_t USBAddr;			   // USB Addr. Default is 0, values of 0..3 are permitted (Only filled if for J-Links connected via USB)
		uint8_t aIPAddr[16];		   // IP Addr. in case emulator is connected via IP. For IP4 (current version), only the first 4 bytes are used.
		int Time;					   // J-Link via IP only: Time period [ms] after which we have received the UDP discover answer from emulator (-1 if emulator is connected over USB)
		uint64_t Time_us;			   // J-Link via IP only: Time period [us] after which we have received the UDP discover answer from emulator (-1 if emulator is connected over USB)
		uint32_t HWVersion;			   // J-Link via IP only: Hardware version of J-Link
		uint8_t abMACAddr[6];		   // J-Link via IP only: MAC Addr
		char acProduct[32];			   // Product name
		char acNickName[32];		   // J-Link via IP only: Nickname of J-Link
		char acFWString[112];		   // J-Link via IP only: Firmware string of J-Link
		char IsDHCPAssignedIP;		   // J-Link via IP only: Is J-Link configured for IP address reception via DHCP?
		char IsDHCPAssignedIPIsValid;  // J-Link via IP only
		char NumIPConnections;		   // J-Link via IP only: Number of IP connections which are currently established to this J-Link
		char NumIPConnectionsIsValid;  // J-Link via IP only
		uint8_t aPadding[34];		   // Pad struct size to 264 bytes
	} JLINKARM_EMU_CONNECT_INFO;	   // In general, unused fields are zeroed.

	typedef struct
	{
		uint32_t Addr;
		uint32_t NumBytes;
		uint32_t Flags;	 // Future use. SBZ.
		uint32_t Dummy;	 // Future use. SBZ.
	} JLINK_HSS_MEM_BLOCK_DESC;

	const char* JLINKARM_OpenEx(JLINKARM_LOG* pfLog, JLINKARM_LOG* pfErrorOut);
	void JLINKARM_Close(void);
	char JLINKARM_IsOpen(void);
	int32_t JLINKARM_ReadMemEx(uint32_t Addr, uint32_t NumBytes, void* pData, uint32_t Flags);
	int32_t JLINKARM_WriteMemEx(uint32_t Addr, uint32_t NumBytes, const void* p, uint32_t Flags);
	int32_t JLINKARM_EMU_GetList(int32_t HostIFs, JLINKARM_EMU_CONNECT_INFO* paConnectInfo, int32_t MaxInfos);
	int32_t JLINKARM_EMU_SelectByUSBSN(uint32_t SerialNo);
	int32_t JLINKARM_ExecCommand(const char* pIn, char* pOut, int32_t BufferSize);
	int32_t JLINKARM_TIF_Select(int32_t int32_terface);
	void JLINKARM_SetSpeed(uint32_t Speed);
	uint16_t JLINKARM_GetSpeed(void);

	int32_t JLINK_HSS_Start(JLINK_HSS_MEM_BLOCK_DESC* paDesc, int32_t NumBlocks, int32_t Period_us, int32_t Flags);
	int32_t JLINK_HSS_Stop(void);
	int32_t JLINK_HSS_Read(void* pBuffer, uint32_t BufferSize);

#if defined(__cplusplus)
} /* Make sure we have C-declarations in C++ programs */
#endif

#endif