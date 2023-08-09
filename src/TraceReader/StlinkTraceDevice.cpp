#include "StlinkTraceDevice.hpp"

#include <cstring>
#include <random>

#include "logging.h"
#include "register.h"

StlinkTraceDevice::StlinkTraceDevice(std::shared_ptr<spdlog::logger> logger) : logger(logger)
{
	init_chipids((char*)"./chips");
}

bool StlinkTraceDevice::stopTrace()
{
	stlink_trace_disable(sl);
	stlink_close(sl);
	return true;
}

bool StlinkTraceDevice::startTrace(uint32_t coreFrequency, uint32_t traceFrequency)
{
	sl = stlink_open_usb(UINFO, CONNECT_HOT_PLUG, NULL, 24000);

	if (sl == nullptr)
		return false;

	stlink_write_debug32(sl, STLINK_REG_DEMCR, STLINK_REG_DEMCR_TRCENA);
	stlink_write_debug32(sl, STLINK_REG_DHCSR,
						 STLINK_REG_DHCSR_DBGKEY | STLINK_REG_DHCSR_C_DEBUGEN |
							 STLINK_REG_DHCSR_C_HALT);
	stlink_write_debug32(sl, STLINK_REG_DEMCR, STLINK_REG_DEMCR_TRCENA);
	stlink_write_debug32(sl, STLINK_REG_CM3_FP_CTRL,
						 STLINK_REG_CM3_FP_CTRL_KEY);
	stlink_write_debug32(sl, STLINK_REG_DWT_FUNCTION0, 0);
	stlink_write_debug32(sl, STLINK_REG_DWT_FUNCTION1, 0);
	stlink_write_debug32(sl, STLINK_REG_DWT_FUNCTION2, 0);
	stlink_write_debug32(sl, STLINK_REG_DWT_FUNCTION3, 0);
	stlink_write_debug32(sl, STLINK_REG_DWT_CTRL, 0);
	stlink_write_debug32(sl, STLINK_REG_DBGMCU_CR,
						 STLINK_REG_DBGMCU_CR_DBG_SLEEP | STLINK_REG_DBGMCU_CR_DBG_STOP |
							 STLINK_REG_DBGMCU_CR_DBG_STANDBY | STLINK_REG_DBGMCU_CR_TRACE_IOEN |
							 STLINK_REG_DBGMCU_CR_TRACE_MODE_ASYNC);

	if (stlink_trace_enable(sl, traceFrequency))
	{
		ELOG("Unable to turn on tracing in stlink\n");
		return false;
	}

	stlink_write_debug32(sl, STLINK_REG_TPI_CSPSR, STLINK_REG_TPI_CSPSR_PORT_SIZE_1);

	if (coreFrequency)
	{
		uint32_t prescaler = coreFrequency / traceFrequency - 1;
		if (prescaler > STLINK_REG_TPI_ACPR_MAX)
		{
			ELOG("Trace frequency prescaler %d out of range.  Try setting a faster trace frequency.\n", prescaler);
			return false;
		}
		stlink_write_debug32(sl, STLINK_REG_TPI_ACPR, prescaler);  // Set TPIU_ACPR clock divisor
	}
	stlink_write_debug32(sl, STLINK_REG_TPI_FFCR, STLINK_REG_TPI_FFCR_TRIG_IN);
	stlink_write_debug32(sl, STLINK_REG_TPI_SPPR, STLINK_REG_TPI_SPPR_SWO_NRZ);
	stlink_write_debug32(sl, STLINK_REG_ITM_LAR, STLINK_REG_ITM_LAR_KEY);
	stlink_write_debug32(sl, STLINK_REG_ITM_TCC, 0x00000400);  // Set sync counter
	stlink_write_debug32(sl, STLINK_REG_ITM_TCR,
						 STLINK_REG_ITM_TCR_TRACE_BUS_ID_1 |
							 STLINK_REG_ITM_TCR_TS_ENA |
							 STLINK_REG_ITM_TCR_ITM_ENA);
	stlink_write_debug32(sl, STLINK_REG_ITM_TER, STLINK_REG_ITM_TER_PORTS_ALL);
	stlink_write_debug32(sl, STLINK_REG_ITM_TPR, STLINK_REG_ITM_TPR_PORTS_ALL);
	stlink_write_debug32(sl, STLINK_REG_DWT_CTRL,
						 4 * STLINK_REG_DWT_CTRL_NUM_COMP |
							 STLINK_REG_DWT_CTRL_CYC_TAP |
							 0xF * STLINK_REG_DWT_CTRL_POST_INIT |
							 0xF * STLINK_REG_DWT_CTRL_POST_PRESET |
							 STLINK_REG_DWT_CTRL_CYCCNT_ENA);

	uint32_t prescaler = 0;
	stlink_read_debug32(sl, STLINK_REG_TPI_ACPR, &prescaler);
	if (prescaler)
	{
		logger->info("Trace prescaler {}", prescaler);
		uint32_t system_clock_speed = (prescaler + 1) * traceFrequency;
		uint32_t system_clock_speed_mhz = (system_clock_speed + 500000) / 1000000;
		ILOG("Trace Port Interface configured to expect a %d MHz system clock.\n",
			 system_clock_speed_mhz);
	}
	else
	{
		WLOG(
			"Trace Port Interface not configured.  Specify the system clock with "
			"a --clock=XX command\n");
		WLOG(
			"line option or set it in your device's clock initialization routine, "
			"such as with:\n");
		WLOG("  TPI->ACPR = HAL_RCC_GetHCLKFreq() / %d - 1;\n", traceFrequency);
	}
	ILOG("Trace frequency set to %d Hz.\n", traceFrequency);

	if (stlink_run(sl, RUN_NORMAL))
	{
		ELOG("Unable to run device\n");
		return false;
	}

	logger->info("Starting reader thread!");

	return true;
}

uint32_t StlinkTraceDevice::readTraceBuffer(uint8_t* buffer, uint32_t size)
{
	return stlink_trace_read(sl, buffer, size);
}