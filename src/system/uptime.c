#include "uptime.h"
#include <stdio.h>
#include <string.h>

long uptime_get_seconds(void) {
    FILE *f = fopen("/proc/uptime", "r");
    if (f == NULL) {
        return -1;
    }
    
    double uptime_seconds;
    if (fscanf(f, "%lf", &uptime_seconds) != 1) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    return (long)uptime_seconds;
}

void uptime_format(long seconds, char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }
    
    long days = seconds / 86400;
    long hours = (seconds % 86400) / 3600;
    long minutes = (seconds % 3600) / 60;
    long secs = seconds % 60;
    
    if (days > 0) {
        snprintf(buffer, size, "%ld 天 %02ld:%02ld:%02ld", days, hours, minutes, secs);
    } else if (hours > 0) {
        snprintf(buffer, size, "%02ld:%02ld:%02ld", hours, minutes, secs);
    } else {
        snprintf(buffer, size, "%02ld:%02ld", minutes, secs);
    }
}
