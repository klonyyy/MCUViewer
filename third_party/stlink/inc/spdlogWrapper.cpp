#include "spdlogWrapper.h"
// clang-format off
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/printf.h>
// clang-format on
#include <stdarg.h>

#include <iostream>

#include "logging.h"

extern std::shared_ptr<spdlog::logger> logger;

int spdlogLog(int level, const char* str, ...)
{
	va_list args;
	va_start(args, str);

	char buf[1000]{};
	vsnprintf(buf, 1000, str, args);
	buf[strlen(buf) - 1] = '\0';

	switch (level)
	{
		case UDEBUG:
			logger->debug(buf);
			break;
		case UINFO:
			logger->info(buf);
			break;
		case UWARN:
			logger->warn(buf);
			break;
		case UERROR:
			logger->error(buf);
			break;
		default:
			logger->info(buf);
			break;
	}
	va_end(args);
	return 1;
}
