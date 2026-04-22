#include "config.h"

#include <cJSON.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_MAX_FILE_SIZE (1024U * 1024U)

static char *dup_string(const char *src)
{
	size_t len;
	char *dst;

	if (!src) {
		return NULL;
	}
	len = strlen(src);
	dst = (char *)malloc(len + 1);
	if (!dst) {
		return NULL;
	}
	memcpy(dst, src, len + 1);
	return dst;
}

static char *read_all(FILE *fp, int *too_large)
{
	size_t used = 0;
	size_t cap = 1024;
	char *buf = (char *)malloc(cap);
	size_t nread;

	if (too_large) {
		*too_large = 0;
	}
	if (!buf) {
		return NULL;
	}

	for (;;) {
		if (used > SIZE_MAX - 512 - 1) {
			free(buf);
			return NULL;
		}
		if (used + 512 + 1 > cap) {
			size_t next_cap = cap * 2;
			if (cap > SIZE_MAX / 2) {
				next_cap = used + 512 + 1;
			}
			if (next_cap < used + 512 + 1) {
				free(buf);
				return NULL;
			}
			char *next = (char *)realloc(buf, next_cap);
			if (!next) {
				free(buf);
				return NULL;
			}
			buf = next;
			cap = next_cap;
		}
		nread = fread(buf + used, 1, 512, fp);
		used += nread;
		if (used > CONFIG_MAX_FILE_SIZE) {
			free(buf);
			if (too_large) {
				*too_large = 1;
			}
			return NULL;
		}
		if (nread < 512) {
			if (ferror(fp)) {
				free(buf);
				return NULL;
			}
			break;
		}
	}
	buf[used] = '\0';
	return buf;
}

static int has_only_known_members(cJSON *obj, const char *const *keys, size_t key_count)
{
	cJSON *item;
	size_t i;
	int found;

	cJSON_ArrayForEach(item, obj) {
		if (!item->string) {
			return 0;
		}
		found = 0;
		for (i = 0; i < key_count; ++i) {
			if (strcmp(item->string, keys[i]) == 0) {
				found = 1;
				break;
			}
		}
		if (!found) {
			return 0;
		}
	}
	return 1;
}

static int parse_interval_seconds(cJSON *root, unsigned int *out_interval)
{
	cJSON *interval_item;
	double interval_double;
	unsigned long interval_sec;

	interval_item = cJSON_GetObjectItemCaseSensitive(root, "interval_seconds");
	if (!cJSON_IsNumber(interval_item)) {
		return CONFIG_ERR_SCHEMA;
	}

	interval_double = cJSON_GetNumberValue(interval_item);
	if (interval_double < 1.0) {
		return CONFIG_ERR_SCHEMA;
	}
	interval_sec = (unsigned long)interval_double;
	if ((double)interval_sec != interval_double || interval_sec > (unsigned long)UINT_MAX) {
		return CONFIG_ERR_SCHEMA;
	}

	*out_interval = (unsigned int)interval_sec;
	return CONFIG_OK;
}

static int parse_zlog_config_path(cJSON *root, char **out_path_copy)
{
	cJSON *path_item;
	const char *path_text;
	char *path_copy;

	path_item = cJSON_GetObjectItemCaseSensitive(root, "zlog_config_path");
	if (!cJSON_IsString(path_item)) {
		return CONFIG_ERR_SCHEMA;
	}
	path_text = cJSON_GetStringValue(path_item);
	if (!path_text || strlen(path_text) == 0) {
		return CONFIG_ERR_SCHEMA;
	}

	path_copy = dup_string(path_text);
	if (!path_copy) {
		return CONFIG_ERR_OOM;
	}

	*out_path_copy = path_copy;
	return CONFIG_OK;
}

static int parse_json_config(struct json_config *cfg, const char *json_text)
{
	static const char *const root_keys[] = {"interval_seconds", "zlog_config_path"};
	cJSON *root;
	unsigned int interval_seconds = 0;
	char *zlog_config_path = NULL;
	int ret;

	root = cJSON_Parse(json_text);
	if (!root) {
		return CONFIG_ERR_PARSE;
	}

	if (!cJSON_IsObject(root) || !has_only_known_members(root, root_keys, 2)) {
		cJSON_Delete(root);
		return CONFIG_ERR_SCHEMA;
	}

	ret = parse_interval_seconds(root, &interval_seconds);
	if (ret != CONFIG_OK) {
		cJSON_Delete(root);
		return ret;
	}

	ret = parse_zlog_config_path(root, &zlog_config_path);
	if (ret != CONFIG_OK) {
		cJSON_Delete(root);
		return ret;
	}

	cfg->interval_seconds = interval_seconds;
	cfg->zlog_config_path = zlog_config_path;

	cJSON_Delete(root);
	return CONFIG_OK;
}

static struct config_load_result config_error_result(int error_code)
{
	struct config_load_result result;

	memset(&result.config, 0, sizeof(result.config));
	result.error_code = error_code;
	return result;
}

struct config_load_result config_load_file(const char *path)
{
	FILE *fp;
	char *json_text;
	int too_large = 0;
	struct config_load_result result = config_error_result(0);

	if (!path) {
		return config_error_result(CONFIG_ERR_INVALID_ARGUMENT);
	}

	fp = fopen(path, "re");
	if (!fp) {
		return config_error_result(CONFIG_ERR_IO);
	}
	json_text = read_all(fp, &too_large);
	fclose(fp);
	if (!json_text) {
		if (too_large) {
			return config_error_result(CONFIG_ERR_TOO_LARGE);
		}
		return config_error_result(CONFIG_ERR_IO);
	}

	result.error_code = parse_json_config(&result.config, json_text);
	if (result.error_code != CONFIG_OK) {
		free(json_text);
		config_free(&result.config);
		return result;
	}

	free(json_text);
	return result;
}

void config_free(struct json_config *cfg)
{
	if (!cfg) {
		return;
	}
	free(cfg->zlog_config_path);
	cfg->zlog_config_path = NULL;
	cfg->interval_seconds = 0;
}
