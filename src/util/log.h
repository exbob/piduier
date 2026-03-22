#ifndef PIDUIER_LOG_H
#define PIDUIER_LOG_H

#include "zlog.h"

/** Initialize zlog from a full zlog-format config string (e.g. from piduier.conf). */
int log_init(const char *zlog_config_string);
void log_fini(void);

extern zlog_category_t *log_cat_piduier;

#define LOG_ERROR(...) zlog_error(log_cat_piduier, __VA_ARGS__)
#define LOG_INFO(...) zlog_info(log_cat_piduier, __VA_ARGS__)
#define LOG_DEBUG(...) zlog_debug(log_cat_piduier, __VA_ARGS__)

#endif
