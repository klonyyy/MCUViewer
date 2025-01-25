#include "spdlogWrapper.h"
// clang-format off
#include <spdlog/spdlog.h>

#if defined(__APPLE__) || defined(__linux__)
	#include <fmt/printf.h>
#elif defined(_WIN32)
	#include <spdlog/fmt/bundled/printf.h>
#else
#error "Your system is not supported!"
#endif

// clang-format on
#include <stdarg.h>

#include <iostream>

#include "logging.h"

extern std::shared_ptr<spdlog::logger> logger;

int spdlogLog(int level, const char* str, ...)
{
	va_list args;
	va_start(args, str);

	char buf[1000];
	int n = vsnprintf(buf, sizeof(buf), str, args);

	if (n >= sizeof(buf))
		buf[sizeof(buf) - 1] = '\0';

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
