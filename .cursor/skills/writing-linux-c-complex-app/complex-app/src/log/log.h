#ifndef _LOG_H
#define _LOG_H

/*
 * log module features:
 * - provides unified logging APIs for application modules
 * - hides third-party zlog implementation details from callers
 * - supports ERROR/WARN/INFO and DEBUG (debug build only)
 */

enum log_error_code {
	LOG_OK = 0,
	LOG_ERR_INVALID_ARGUMENT = 1,
	LOG_ERR_INIT_FAILED = 2
};

enum log_level {
	LOG_LEVEL_ERR = 0,
	LOG_LEVEL_WARN = 1,
	LOG_LEVEL_INFO = 2,
	LOG_LEVEL_DEBUG = 3
};

/**
 * @brief Initialize logging backend with zlog config file.
 * @param zlog_config_path Path to zlog ini configuration file.
 * @return LOG_OK on success, otherwise one of log_error_code values.
 */
int log_init(const char *zlog_config_path);

/**
 * @brief Shutdown logging backend and release related resources.
 */
void log_shutdown(void);

/**
 * @brief Write one log message with location metadata.
 * @param level Log severity level.
 * @param file Source file name (__FILE__).
 * @param func Function name (__func__).
 * @param line Source line number (__LINE__).
 * @param fmt  printf-style format string.
 * @param ...  printf-style format arguments.
 */
void log_write(enum log_level level, const char *file, const char *func, long line, const char *fmt, ...);

#define LOG_ERR(...)	log_write(LOG_LEVEL_ERR, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)	log_write(LOG_LEVEL_WARN, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)	log_write(LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__)

#ifdef APP_DEBUG
#define LOG_DEBUG(...) log_write(LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif /* _LOG_H */
