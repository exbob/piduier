#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 全局变量用于缓存 CPU 时间
static cpu_time_t cached_prev = {0};
static cpu_time_t cached_curr = {0};
static double cached_usage = 0.0;
static int cache_valid = 0;
static int cache_initialized = 0;

int cpu_read_time(cpu_time_t *cpu) {
    if (cpu == NULL) {
        return -1;
    }
    
    FILE *f = fopen("/proc/stat", "r");
    if (f == NULL) {
        return -1;
    }
    
    char line[256];
    if (fgets(line, sizeof(line), f) == NULL) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    
    if (sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &cpu->user, &cpu->nice, &cpu->system, &cpu->idle,
               &cpu->iowait, &cpu->irq, &cpu->softirq, &cpu->steal) != 8) {
        return -1;
    }
    
    return 0;
}

double cpu_calculate_usage(const cpu_time_t *prev, const cpu_time_t *curr) {
    if (prev == NULL || curr == NULL) {
        return -1.0;
    }
    
    unsigned long prev_total = prev->user + prev->nice + prev->system +
                               prev->idle + prev->iowait + prev->irq +
                               prev->softirq + prev->steal;
    unsigned long curr_total = curr->user + curr->nice + curr->system +
                               curr->idle + curr->iowait + curr->irq +
                               curr->softirq + curr->steal;
    
    unsigned long prev_idle = prev->idle + prev->iowait;
    unsigned long curr_idle = curr->idle + curr->iowait;
    
    unsigned long total_delta = curr_total - prev_total;
    unsigned long idle_delta = curr_idle - prev_idle;
    
    if (total_delta == 0) {
        return 0.0;
    }
    
    double usage = 100.0 * (1.0 - (double)idle_delta / (double)total_delta);
    
    if (usage < 0.0) usage = 0.0;
    if (usage > 100.0) usage = 100.0;
    
    return usage;
}

double cpu_get_usage(void) {
    if (cache_valid) {
        return cached_usage;
    }
    
    if (!cache_initialized) {
        if (cpu_read_time(&cached_prev) != 0) {
            return -1.0;
        }
        cache_initialized = 1;
        return 0.0;
    }
    
    return cached_usage >= 0 ? cached_usage : 0.0;
}

void cpu_update_cache(void) {
    if (!cache_initialized) {
        if (cpu_read_time(&cached_prev) == 0) {
            cache_initialized = 1;
        }
        return;
    }
    
    cached_prev = cached_curr;
    
    if (cpu_read_time(&cached_curr) == 0) {
        cached_usage = cpu_calculate_usage(&cached_prev, &cached_curr);
        cache_valid = 1;
    }
}
