/*
 * Ugly, low performance, configurable level, logging "framework"
 */

#ifndef UGLYLOGGING_H
#define UGLYLOGGING_H

#include "spdlogWrapper.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum ugly_loglevel {
    UDEBUG = 90,
    UINFO  = 50,
    UWARN  = 30,
    UERROR = 20
};

#if defined(__GNUC__)
#define PRINTF_ARRT __attribute__ ((format (printf, 3, 4)))
#else
#define PRINTF_ARRT
#endif

int ugly_init(int maximum_threshold);
int ugly_log(int level, const char *tag, const char *format, ...) PRINTF_ARRT;
int ugly_libusb_log_level(enum ugly_loglevel v);

#define UGLY_LOG_FILE (strstr(__FILE__, "/") != NULL ? \
                       strrchr(__FILE__, '/')  + 1 : strstr(__FILE__, "\\") != NULL ? \
                       strrchr(__FILE__, '\\') + 1 : __FILE__)

#define DLOG(...) spdlogLog(UDEBUG, __VA_ARGS__)
#define ILOG(...) spdlogLog(UINFO, __VA_ARGS__)
#define WLOG(...) spdlogLog(UWARN, __VA_ARGS__)
#define ELOG(...) spdlogLog(UERROR, __VA_ARGS__)

#ifdef  __cplusplus
}
#endif

#endif  // UGLYLOGGING_H
