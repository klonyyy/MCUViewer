#include "StlinkTraceDevice.hpp"

#include <cstring>
#include <random>

#include "logging.h"
#include "register.h"

StlinkTraceDevice::StlinkTraceDevice(std::shared_ptr<spdlog::logger> logger) : logger(logger)
{
	init_chipids(const_cast<char*>("./chips"));
}

bool StlinkTraceDevice::stopTrace()
{
	if (sl == nullptr)
		return false;

	stlink_exit_debug_mode(sl);
	stlink_trace_disable(sl);
	stlink_close(sl);

	logger->info("Trace stopped.");
	return true;
}

bool StlinkTraceDevice::startTrace(uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset)
{
	sl = stlink_open_usb(UINFO, CONNECT_HOT_PLUG, NULL, 24000);

	if (sl == nullptr)
	{
		logger->error("Stlink not found!");
		return false;
	}

	if (shouldReset)
		stlink_reset(sl, RESET_SOFT);

	/* turn on DWT and ITM */
	stlink_write_debug32(sl, STLINK_REG_DEMCR, STLINK_REG_DEMCR_TRCENA);
	stlink_write_debug32(sl, STLINK_REG_DHCSR, STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN | STLINK_REG_DHCSR_C_HALT);
	stlink_write_debug32(sl, STLINK_REG_DWT_FUNCTION0, 0);
	stlink_write_debug32(sl, STLINK_REG_DWT_FUNCTION1, 0);
	stlink_write_debug32(sl, STLINK_REG_DWT_FUNCTION2, 0);
	stlink_write_debug32(sl, STLINK_REG_DWT_FUNCTION3, 0);
	stlink_write_debug32(sl, STLINK_REG_DWT_CTRL, 0);
	stlink_write_debug32(sl, STLINK_REG_DBGMCU_CR, STLINK_REG_DBGMCU_CR_TRACE_IOEN | STLINK_REG_DBGMCU_CR_TRACE_MODE_ASYNC);

	uint32_t traceFrequency = coreFrequency / (tracePrescaler + 1);

	logger->info("Trace frequency {}", traceFrequency);
	logger->info("Trace prescaler {}", tracePrescaler);
	logger->info("Trace channels mask {}", activeChannelMask);

	if (stlink_trace_enable(sl, traceFrequency))
	{
		logger->error("Unable to turn on tracing in stlink");
		return false;
	}

	stlink_write_debug32(sl, STLINK_REG_TPI_CSPSR, STLINK_REG_TPI_CSPSR_PORT_SIZE_1);

	stlink_write_debug32(sl, STLINK_REG_TPI_ACPR, tracePrescaler);
	stlink_write_debug32(sl, STLINK_REG_TPI_FFCR, STLINK_REG_TPI_FFCR_TRIG_IN);
	stlink_write_debug32(sl, STLINK_REG_TPI_SPPR, STLINK_REG_TPI_SPPR_SWO_NRZ);
	stlink_write_debug32(sl, STLINK_REG_ITM_LAR, STLINK_REG_ITM_LAR_KEY);
	stlink_write_debug32(sl, STLINK_REG_ITM_TCR, STLINK_REG_ITM_TCR_TRACE_BUS_ID_1 | STLINK_REG_ITM_TCR_TS_ENA | STLINK_REG_ITM_TCR_ITM_ENA);
	stlink_write_debug32(sl, STLINK_REG_ITM_TER, activeChannelMask);
	stlink_write_debug32(sl, STLINK_REG_ITM_TPR, 0x000F);

	if (stlink_run(sl, RUN_NORMAL))
	{
		logger->error("Unable to run target device");
		return false;
	}

	logger->info("Starting reader thread!");

	return true;
}

int32_t StlinkTraceDevice::readTraceBuffer(uint8_t* buffer, uint32_t size)
{
	if (sl == nullptr)
		return -1;

	return stlink_trace_read(sl, buffer, size);
}