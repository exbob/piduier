#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int memory_get_info(memory_info_t *info)
{
	if (info == NULL) {
		return -1;
	}

	memset(info, 0, sizeof(memory_info_t));

	FILE *f = fopen("/proc/meminfo", "r");
	if (f == NULL) {
		return -1;
	}

	char line[256];
	while (fgets(line, sizeof(line), f) != NULL) {
		if (strncmp(line, "MemTotal:", 9) == 0) {
			sscanf(line, "MemTotal: %lu kB", &info->total);
		} else if (strncmp(line, "MemFree:", 8) == 0) {
			sscanf(line, "MemFree: %lu kB", &info->free);
		} else if (strncmp(line, "MemAvailable:", 13) == 0) {
			sscanf(line, "MemAvailable: %lu kB", &info->available);
		}
	}

	fclose(f);

	if (info->available > 0) {
		info->used = info->total - info->available;
	} else {
		info->used = info->total - info->free;
	}

	if (info->total > 0) {
		info->usage_percent = (double)info->used / (double)info->total * 100.0;
	} else {
		info->usage_percent = 0.0;
	}

	return 0;
}

char *memory_info_to_json(const memory_info_t *info)
{
	if (info == NULL) {
		return NULL;
	}

	size_t size = 256;
	char *json = malloc(size);
	if (json == NULL) {
		return NULL;
	}

	snprintf(json, size,
	         "{"
	         "\"total\":%lu,"
	         "\"used\":%lu,"
	         "\"free\":%lu,"
	         "\"available\":%lu,"
	         "\"usage_percent\":%.2f"
	         "}",
	         info->total, info->used, info->free, info->available,
	         info->usage_percent);

	return json;
}
