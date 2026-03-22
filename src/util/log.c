#include "log.h"
#include <stdio.h>
#include <stdlib.h>

zlog_category_t *log_cat_piduier;

int log_init(const char *conf_path) {
    if (zlog_init(conf_path) != 0) {
        fprintf(stderr, "zlog_init failed for config: %s\n",
                conf_path ? conf_path : "(null)");
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
