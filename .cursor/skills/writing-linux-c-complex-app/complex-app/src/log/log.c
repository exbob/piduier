#include "log.h"

#include <stdarg.h>
#include <string.h>
#include <zlog.h>

static int to_zlog_level(enum log_level level)
{
	switch (level) {
	case LOG_LEVEL_ERR:
		return ZLOG_LEVEL_ERROR;
	case LOG_LEVEL_WARN:
		return ZLOG_LEVEL_WARN;
	case LOG_LEVEL_DEBUG:
		return ZLOG_LEVEL_DEBUG;
	case LOG_LEVEL_INFO:
	default:
		return ZLOG_LEVEL_INFO;
	}
}

int log_init(const char *zlog_config_path)
{
	if (!zlog_config_path || zlog_config_path[0] == '\0') {
		return LOG_ERR_INVALID_ARGUMENT;
	}
	if (dzlog_init(zlog_config_path, "app") != 0) {
		return LOG_ERR_INIT_FAILED;
	}
	return LOG_OK;
}

void log_shutdown(void)
{
	zlog_fini();
}

void log_write(enum log_level level, const char *file, const char *func, long line, const char *fmt, ...)
{
	va_list args;
	int zlog_level;

	if (!file || !func || !fmt) {
		return;
	}
	zlog_level = to_zlog_level(level);
	va_start(args, fmt);
	vdzlog(file, strlen(file), func, strlen(func), line, zlog_level, fmt, args);
	va_end(args);
}
