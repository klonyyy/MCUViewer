/*
 * File: logging.h
 *
 * UglyLogging: Slow, yet another wheel reinvented, but enough to make the rest of our code pretty enough.
 * Ugly, low performance, configurable level, logging "framework"
 */

#ifndef LOGGING_H
#define LOGGING_H

#include "spdlogWrapper.h"

#ifdef  __cplusplus
extern "C" {
#endif // __cplusplus

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
#endif // __GNUC__

int32_t ugly_init(int32_t maximum_threshold);
int32_t ugly_log(int32_t level, const char *tag, const char *format, ...) PRINTF_ARRT;
int32_t ugly_libusb_log_level(enum ugly_loglevel v);

#define UGLY_LOG_FILE (strstr(__FILE__, "/") != NULL ? \
                       strrchr(__FILE__, '/')  + 1 : strstr(__FILE__, "\\") != NULL ? \
                       strrchr(__FILE__, '\\') + 1 : __FILE__)

// TODO: we need to write this in a more generic way, for now this should compile
// on visual studio (See http://stackoverflow.com/a/8673872/1836746)
#define DLOG(...) spdlogLog(UDEBUG, __VA_ARGS__)
#define ILOG(...) spdlogLog(UINFO, __VA_ARGS__)
#define WLOG(...) spdlogLog(UWARN, __VA_ARGS__)
#define ELOG(...) spdlogLog(UERROR, __VA_ARGS__)

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // LOGGING_H
