#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * config module features:
 * - loads app configuration from a JSON file
 * - enforces config file size limit: 1MB
 * - validates schema and value constraints during parsing
 * - returns structured data and explicit error codes
 */

struct json_config {
	unsigned int interval_seconds;
	char *zlog_config_path;
};

/* Error codes returned by config module APIs. */
enum config_error_code {
	CONFIG_OK = 0,                   /* Success. */
	CONFIG_ERR_INVALID_ARGUMENT = 1, /* Invalid input argument (e.g. NULL path). */
	CONFIG_ERR_IO = 2,               /* File read/open error. */
	CONFIG_ERR_PARSE = 3,            /* JSON parse failed. */
	CONFIG_ERR_OOM = 4,              /* Memory allocation failed. */
	CONFIG_ERR_SCHEMA = 5,           /* JSON schema or value constraints invalid. */
	CONFIG_ERR_TOO_LARGE = 6         /* Config file exceeds 1MB limit. */
};

struct config_load_result {
	int error_code;
	struct json_config config;
};

/**
 * @brief Load configuration from a specified JSON file.
 * @param path Path to the JSON configuration file.
 * @return The result of configuration loading, including error code and parsed config.
 */
struct config_load_result config_load_file(const char *path);

/**
 * @brief Free any resources allocated within the json_config struct.
 * @param cfg Pointer to the json_config struct to free.
 */
void config_free(struct json_config *cfg);

#endif /* _CONFIG_H */
