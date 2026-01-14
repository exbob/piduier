#include "temperature.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEMP_FILE "/sys/class/thermal/thermal_zone0/temp"
#define MIN_TEMP -50.0
#define MAX_TEMP 150.0

double temperature_get_cpu(void) {
    FILE *f = fopen(TEMP_FILE, "r");
    if (f == NULL) {
        return -1.0;
    }
    
    char line[64];
    if (fgets(line, sizeof(line), f) == NULL) {
        fclose(f);
        return -1.0;
    }
    
    fclose(f);
    
    long temp_millidegrees = 0;
    if (sscanf(line, "%ld", &temp_millidegrees) != 1) {
        return -1.0;
    }
    
    double temp_celsius = (double)temp_millidegrees / 1000.0;
    
    if (temp_celsius < MIN_TEMP || temp_celsius > MAX_TEMP) {
        return -1.0;
    }
    
    return temp_celsius;
}
