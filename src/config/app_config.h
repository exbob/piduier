#ifndef PIDUIER_APP_CONFIG_H
#define PIDUIER_APP_CONFIG_H

#include <stddef.h>

typedef struct piduier_config {
    char http_listen[256];
    int http_port;
    /** zlog.log_file as read from JSON (may be empty when no @LOG_FILE@ in rules) */
    char log_file[1024];
    /** malloc'd zlog ini text for zlog_init_from_string; free with piduier_config_free */
    char *zlog_ini;
} piduier_config_t;

/**
 * Load and validate piduier.conf (JSON). On failure returns -1 and writes a message to errbuf.
 * On success returns 0; caller must call piduier_config_free when done.
 */
int piduier_config_load(const char *path, piduier_config_t *cfg,
                        char *errbuf, size_t errbuf_sz);

void piduier_config_free(piduier_config_t *cfg);

#endif
