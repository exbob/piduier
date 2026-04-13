#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int storage_get_info(const char *device, storage_info_t *info)
{
	if (info == NULL) {
		return -1;
	}

	memset(info, 0, sizeof(storage_info_t));

	char cmd[512];
	if (device != NULL && strlen(device) > 0) {
		snprintf(cmd, sizeof(cmd), "df -B1 %s 2>/dev/null", device);
	} else {
		snprintf(cmd, sizeof(cmd), "df -B1 / 2>/dev/null");
	}

	FILE *fp = popen(cmd, "r");
	if (fp == NULL) {
		return -1;
	}

	char line[512];
	int header_found = 0;
	int data_found = 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (!header_found) {
			if (strstr(line, "Filesystem") != NULL &&
			    strstr(line, "1B-blocks") != NULL) {
				header_found = 1;
			}
			continue;
		}

		char filesystem[256];
		unsigned long long blocks, used, available;
		int use_percent;
		char mount_point[256];

		if (sscanf(line, "%255s %llu %llu %llu %d%% %255s", filesystem, &blocks,
		           &used, &available, &use_percent, mount_point) == 6) {
			info->total = blocks;
			info->used = used;
			info->available = available;
			info->usage_percent = (double)use_percent;

			size_t device_len = strlen(filesystem);
			if (device_len >= sizeof(info->device)) {
				device_len = sizeof(info->device) - 1;
			}
			memcpy(info->device, filesystem, device_len);
			info->device[device_len] = '\0';

			size_t mount_len = strlen(mount_point);
			if (mount_len >= sizeof(info->mount_point)) {
				mount_len = sizeof(info->mount_point) - 1;
			}
			memcpy(info->mount_point, mount_point, mount_len);
			info->mount_point[mount_len] = '\0';

			data_found = 1;
			break;
		}
	}

	pclose(fp);

	if (!data_found) {
		return -1;
	}

	if (info->total == 0) {
		return -1;
	}

	if (info->usage_percent < 0.0 || info->usage_percent > 100.0) {
		if (info->total > 0) {
			info->usage_percent =
			    (double)info->used / (double)info->total * 100.0;
		} else {
			info->usage_percent = 0.0;
		}
	}

	return 0;
}

char *storage_info_to_json(const storage_info_t *info)
{
	if (info == NULL) {
		return NULL;
	}

	size_t size = 512;
	char *json = malloc(size);
	if (json == NULL) {
		return NULL;
	}

	snprintf(json, size,
	         "{"
	         "\"total\":%llu,"
	         "\"used\":%llu,"
	         "\"available\":%llu,"
	         "\"usage_percent\":%.2f,"
	         "\"device\":\"%s\","
	         "\"mount_point\":\"%s\""
	         "}",
	         info->total, info->used, info->available, info->usage_percent,
	         info->device, info->mount_point);

	return json;
}
