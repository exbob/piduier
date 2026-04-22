#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>

#if defined(APP_DEBUG)
#define LOG_ERR(fmt, ...) \
	fprintf(stdout, "[ERR] %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
	fprintf(stdout, "[WARN] %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
	fprintf(stdout, "[INFO] %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
	fprintf(stdout, "[DEBUG] %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_ERR(fmt, ...) \
	fprintf(stdout, "[ERR] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
	fprintf(stdout, "[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
	fprintf(stdout, "[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) do { } while (0)
#endif

#endif /* _LOG_H */
