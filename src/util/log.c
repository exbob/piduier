#include "log.h"
#include <stdio.h>
#include <stdlib.h>

zlog_category_t *log_cat_piduier;

int log_init(const char *zlog_config_string) {
    if (zlog_config_string == NULL) {
        fprintf(stderr, "zlog: config string is null\n");
        return -1;
    }
    if (zlog_init_from_string(zlog_config_string) != 0) {
        fprintf(stderr, "zlog_init_from_string failed\n");
        return -1;
    }
    log_cat_piduier = zlog_get_category("piduier");
    if (log_cat_piduier == NULL) {
        fprintf(stderr, "zlog_get_category(piduier) failed\n");
        zlog_fini();
        return -1;
    }
    (void)zlog_get_category("mongoose");
    return 0;
}

void log_fini(void) {
    zlog_fini();
    log_cat_piduier = NULL;
}
