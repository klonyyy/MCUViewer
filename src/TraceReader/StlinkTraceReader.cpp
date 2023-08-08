#include "StlinkTraceReader.hpp"

#include <cstring>
#include <random>

#include "logging.h"
#include "register.h"

#define TRACE_OP_IS_OVERFLOW(c)		   (c == 0x70)
#define TRACE_OP_IS_LOCAL_TIME(c)	   ((c & 0x0f) == 0x00 && (c & 0x70) != 0x00)
#define TRACE_OP_IS_EXTENSION(c)	   ((c & 0x0b) == 0x08)
#define TRACE_OP_IS_GLOBAL_TIME(c)	   ((c & 0xdf) == 0x94)
#define TRACE_OP_IS_SOURCE(c)		   ((c & 0x03) != 0x00)
#define TRACE_OP_IS_SW_SOURCE(c)	   ((c & 0x03) != 0x00 && (c & 0x04) == 0x00)
#define TRACE_OP_IS_HW_SOURCE(c)	   ((c & 0x03) != 0x00 && (c & 0x04) == 0x04)
#define TRACE_OP_IS_TARGET_SOURCE(c)   (c & 0x01)
#define TRACE_OP_GET_CONTINUATION(c)   (c & 0x80)
#define TRACE_OP_GET_SOURCE_SIZE(c)	   (c & 0x03)
#define TRACE_OP_GET_SW_SOURCE_ADDR(c) (c >> 3)

StlinkTraceReader::StlinkTraceReader(std::shared_ptr<spdlog::logger> logger) : logger(logger)
{
	init_chipids((char*)"./chips");
}

bool StlinkTraceReader::startAcqusition()
{
	sl = stlink_open_usb(UINFO, CONNECT_HOT_PLUG, NULL, 24000);
	isRunning = false;

	if (sl == nullptr)
	{
		lastErrorMsg = "STLink not found!";
		return false;
	}

	isRunning = true;
	if (!enableTrace())
	{
		isRunning = false;
		lastErrorMsg = "Error running trace";
		return false;
	}
	logger->info("Starting trace acqusition");

	lastErrorMsg = "";
	return true;
}
bool StlinkTraceReader::stopAcqusition()
{
	logger->info("Stopping trace acqusition");
	traceTable.pop();
	isRunning = false;
	if (readerHandle.joinable())
		readerHandle.join();

	stlink_trace_disable(sl);
	stlink_close(sl);
	return true;
}

bool StlinkTraceReader::isValid() const
{
	return isRunning;
}

bool StlinkTraceReader::readTrace(double& timestamp, std::array<bool, 10>& trace)
{
	auto entry = traceTable.pop();
	timestamp = entry.second / static_cast<double>(coreFrequency);
	trace = entry.first;
	return true;
}

std::string StlinkTraceReader::getLastErrorMsg() const
{
	return lastErrorMsg;
}

void StlinkTraceReader::setCoreClockFrequency(uint32_t frequencyHz)
{
	coreFrequency = frequencyHz;
}

uint32_t StlinkTraceReader::getCoreClockFrequency() const
{
	return coreFrequency;
}

void StlinkTraceReader::setTraceFrequency(uint32_t frequencyHz)
{
	traceFrequency = frequencyHz;
}

uint32_t StlinkTraceReader::getTraceFrequency() const
{
	return traceFrequency;
}

bool StlinkTraceReader::enableTrace()
{
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

	uint32_t prescaler;
	stlink_read_debug32(sl, STLINK_REG_TPI_ACPR_MAX, &prescaler);
	if (prescaler)
	{
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

	readerHandle = std::thread(&StlinkTraceReader::readerThread, this);

	return true;
}

StlinkTraceReader::TraceState StlinkTraceReader::updateTraceIdle(uint8_t c)
{
	if (TRACE_OP_IS_TARGET_SOURCE(c))
	{
		currentChannel[awaitingTimestamp] = (c & 0xf8) >> 3;
		return TRACE_STATE_TARGET_SOURCE;
	}
	else if (TRACE_OP_IS_LOCAL_TIME(c) || TRACE_OP_IS_GLOBAL_TIME(c))
	{
		memset(timestampBuf, 0, sizeof(timestampBuf));
		timestamp = 0;
		timestampBytes = 0;

		if ((c & 0x30) != 0x00)
			WLOG("Possible delay in timestamp generation!\r\n");

		if (TRACE_OP_GET_CONTINUATION(c))
			return TRACE_STATE_TARGET_TIMESTAMP_HEADER;
		else
		{
			timestampBuf[timestampBytes++] = c;
			timestampEnd();
			return TRACE_STATE_IDLE;
		}
	}
	else if (TRACE_OP_IS_EXTENSION(c))
		return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME : TRACE_STATE_IDLE;
	else if (TRACE_OP_IS_OVERFLOW(c))
		WLOG("OVERFLOW OPTCODE 0x%02x\n", c);

	WLOG("Unknown opcode 0x%02x\n", c);

	errorCount++;
	return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME : TRACE_STATE_IDLE;
}

void StlinkTraceReader::timestampEnd()
{
	if (timestampBytes == 1)
		timestamp = (uint32_t)(timestampBuf[0] & 0x7f) >> 4;
	else
	{
		for (uint32_t i = 0; i < timestampBytes; i++)
			timestamp |= (uint32_t)(timestampBuf[i] & 0x7f) << 7 * i;
	}

	std::array<bool, channels> currentEntry{previousEntry};

	uint32_t i = 0;
	while (awaitingTimestamp--)
	{
		currentEntry[currentChannel[i]] = currentValue[i] == 0xaa ? true : false;
		i++;
	}

	traceTable.push(std::pair<std::array<bool, channels>, uint32_t>{currentEntry, timestamp});
	previousEntry = currentEntry;
	awaitingTimestamp = 0;
}

StlinkTraceReader::TraceState StlinkTraceReader::updateTrace(uint8_t c)
{
	if (state == TRACE_STATE_UNKNOWN)
	{
		WLOG("STATE UNKNOWN \r\n", c);
		if (TRACE_OP_IS_TARGET_SOURCE(c) || TRACE_OP_IS_LOCAL_TIME(c) || TRACE_OP_IS_GLOBAL_TIME(c))
			state = TRACE_STATE_IDLE;
	}

	switch (state)
	{
		case TRACE_STATE_IDLE:
		{
			return updateTraceIdle(c);
		}

		case TRACE_STATE_TARGET_SOURCE:
		{
			currentValue[awaitingTimestamp++] = c;
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_TARGET_TIMESTAMP_HEADER:
		{
			timestampBuf[timestampBytes++] = c;
			if (TRACE_OP_GET_CONTINUATION(c))
				return TRACE_STATE_TARGET_TIMESTAMP_CONT;
			else
				timestampEnd();
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_TARGET_TIMESTAMP_CONT:
		{
			timestampBuf[timestampBytes++] = c;
			if (TRACE_OP_GET_CONTINUATION(c))
				return TRACE_STATE_TARGET_TIMESTAMP_CONT;
			else
				timestampEnd();
			return TRACE_STATE_IDLE;
		}

		case TRACE_STATE_SKIP_FRAME:
			return TRACE_OP_GET_CONTINUATION(c) ? TRACE_STATE_SKIP_FRAME
												: TRACE_STATE_IDLE;
		case TRACE_STATE_SKIP_4:
			return TRACE_STATE_SKIP_3;

		case TRACE_STATE_SKIP_3:
			return TRACE_STATE_SKIP_2;

		case TRACE_STATE_SKIP_2:
			return TRACE_STATE_SKIP_1;

		case TRACE_STATE_SKIP_1:
			return TRACE_STATE_IDLE;

		case TRACE_STATE_UNKNOWN:
			return TRACE_STATE_UNKNOWN;

		default:
			ELOG("Invalid state %d.  This should never happen\n", state);
			return TRACE_STATE_IDLE;
	}
}

void StlinkTraceReader::readerThread()
{
	while (isRunning)
	{
		uint8_t buffer[STLINK_TRACE_BUF_LEN];
		int length = stlink_trace_read(sl, buffer, sizeof(buffer));

		if (length == 0)
		{
			ILOG("SLEEP");
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			continue;
		}

		if (length == sizeof(buffer))
		{
			ELOG("OVERFLOW");
			continue;
		}

		for (int i = 0; i < length; i++)
		{
			state = updateTrace(buffer[i]);
			if (!isRunning)
				break;
		}
	}
}