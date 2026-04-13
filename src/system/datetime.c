#include "datetime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void parse_timedatectl_line(const char *line, datetime_info_t *info)
{
	if (strstr(line, "Local time:") != NULL) {
		char *colon = strchr(line, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			char *date_start = colon;
			while (*date_start != ' ' && *date_start != '\0')
				date_start++;
			while (*date_start == ' ')
				date_start++;
			if (sscanf(date_start, "%10s %8s", info->local_time,
			           info->local_time + 11) == 2) {
				info->local_time[10] = ' ';
				info->local_time[19] = '\0';
			}
		}
	} else if (strstr(line, "Universal time:") != NULL) {
		char *colon = strchr(line, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			char *date_start = colon;
			while (*date_start != ' ' && *date_start != '\0')
				date_start++;
			while (*date_start == ' ')
				date_start++;
			if (sscanf(date_start, "%10s %8s", info->utc_time,
			           info->utc_time + 11) == 2) {
				info->utc_time[10] = ' ';
				info->utc_time[19] = '\0';
			}
		}
	} else if (strstr(line, "Time zone:") != NULL) {
		char *colon = strchr(line, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%63[^ (]", info->timezone);
		}
		char *offset_start = strchr(line, '+');
		if (!offset_start)
			offset_start = strchr(line, '-');
		if (offset_start) {
			sscanf(offset_start, "%15s", info->timezone_offset);
		}
	} else if (strstr(line, "System clock synchronized:") != NULL) {
		info->ntp_synchronized = strstr(line, "yes") != NULL ? 1 : 0;
	} else if (strstr(line, "NTP service:") != NULL) {
		info->ntp_service_active = strstr(line, "active") != NULL ? 1 : 0;
	}
}

int datetime_get_info(datetime_info_t *info)
{
	if (info == NULL) {
		return -1;
	}

	memset(info, 0, sizeof(datetime_info_t));

	FILE *fp = popen("timedatectl status 2>/dev/null", "r");
	if (fp == NULL) {
		return -1;
	}

	char line[512];
	while (fgets(line, sizeof(line), fp) != NULL) {
		parse_timedatectl_line(line, info);
	}

	pclose(fp);
	return 0;
}

int datetime_set_time(const char *datetime)
{
	if (datetime == NULL || strlen(datetime) == 0) {
		return -1;
	}

	for (const char *p = datetime; *p; p++) {
		if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' ||
		    *p == '(' || *p == ')' || *p == '<' || *p == '>') {
			return -1;
		}
	}

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "timedatectl set-time \"%s\" 2>/dev/null",
	         datetime);
	int result = system(cmd);
	return (result == 0) ? 0 : -1;
}

int datetime_set_timezone(const char *timezone)
{
	if (timezone == NULL || strlen(timezone) == 0 || strlen(timezone) > 64) {
		return -1;
	}

	for (const char *p = timezone; *p; p++) {
		if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' ||
		    *p == '(' || *p == ')' || *p == '<' || *p == '>') {
			return -1;
		}
	}

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "timedatectl set-timezone %s 2>/dev/null",
	         timezone);
	int result = system(cmd);
	return (result == 0) ? 0 : -1;
}

char **datetime_get_timezones(size_t *count)
{
	FILE *fp = popen("timedatectl list-timezones 2>/dev/null", "r");
	if (fp == NULL) {
		if (count)
			*count = 0;
		return NULL;
	}

	// 先计算行数
	size_t capacity = 100;
	size_t size = 0;
	char **timezones = malloc(capacity * sizeof(char *));
	if (timezones == NULL) {
		pclose(fp);
		if (count)
			*count = 0;
		return NULL;
	}

	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL) {
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';
		}

		if (len > 1) {
			if (size >= capacity) {
				capacity *= 2;
				timezones = realloc(timezones, capacity * sizeof(char *));
				if (timezones == NULL) {
					pclose(fp);
					datetime_free_timezones(timezones);
					if (count)
						*count = 0;
					return NULL;
				}
			}

			timezones[size] = malloc(strlen(line) + 1);
			if (timezones[size] == NULL) {
				pclose(fp);
				datetime_free_timezones(timezones);
				if (count)
					*count = 0;
				return NULL;
			}
			strcpy(timezones[size], line);
			size++;
		}
	}

	pclose(fp);

	if (size >= capacity) {
		timezones = realloc(timezones, (size + 1) * sizeof(char *));
	}
	timezones[size] = NULL;

	if (count)
		*count = size;
	return timezones;
}

void datetime_free_timezones(char **timezones)
{
	if (timezones == NULL) {
		return;
	}

	for (size_t i = 0; timezones[i] != NULL; i++) {
		free(timezones[i]);
	}
	free(timezones);
}

char *datetime_info_to_json(const datetime_info_t *info)
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
	         "\"local\":\"%s\","
	         "\"utc\":\"%s\","
	         "\"timezone\":\"%s\","
	         "\"timezone_offset\":\"%s\","
	         "\"ntp_synchronized\":%d,"
	         "\"ntp_service\":\"%s\""
	         "}",
	         info->local_time, info->utc_time, info->timezone,
	         info->timezone_offset, info->ntp_synchronized,
	         info->ntp_service_active ? "active" : "inactive");

	return json;
}
