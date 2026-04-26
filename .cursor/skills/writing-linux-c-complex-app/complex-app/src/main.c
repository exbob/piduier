#include "Config.h"
#include "app.h"
#include "config.h"
#include "log.h"

#include <getopt.h>
#include <stdio.h>

static struct option long_options[] = {
	{"config", required_argument, NULL, 'f'},
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{0, 0, 0, 0}
};

static void print_help(const char *prog)
{
	printf("Usage: %s [OPTION]\n", prog);
	printf("A Linux C complex-app demo application.\n\n");
	printf("Options:\n");
	printf("  -f, --config   Load JSON config from file\n");
	printf("  -h, --help     Show this help message\n");
	printf("  -v, --version  Show program version information\n");
}

static void print_version(void)
{
	printf("Program: complex-app\n");
	printf("Description: A Linux C complex-app demo application.\n");
	printf("Version: %s\n", VERSION);
	printf("Build time(UTC): %s\n", BUILD_TIME);
}

static const char *config_error_to_string(int error_code)
{
	switch (error_code) {
	case CONFIG_ERR_INVALID_ARGUMENT:
		return "invalid argument";
	case CONFIG_ERR_IO:
		return "io error";
	case CONFIG_ERR_PARSE:
		return "json parse error";
	case CONFIG_ERR_OOM:
		return "out of memory";
	case CONFIG_ERR_SCHEMA:
		return "config schema invalid";
	case CONFIG_ERR_TOO_LARGE:
		return "config file too large (max 1MB)";
	default:
		return "unknown error";
	}
}

static const char *log_error_to_string(int error_code)
{
	switch (error_code) {
	case LOG_ERR_INVALID_ARGUMENT:
		return "invalid zlog config path";
	case LOG_ERR_INIT_FAILED:
		return "zlog init failed";
	default:
		return "unknown error";
	}
}

int main(int argc, char *argv[])
{
	struct config_load_result load_result;
	const char *config_path = "app.json";
	int c;
	int log_ret;

	opterr = 0;

	// Parse command line arguments
	while ((c = getopt_long(argc, argv, "f:hv", long_options, NULL)) != -1) {
		switch (c) {
		case 'f':
			config_path = optarg;
			break;
		case 'h':
			print_help(argv[0]);
			return 0;
		case 'v':
			print_version();
			return 0;
		default:
			print_help(argv[0]);
			return 1;
		}
	}

	// Check for unexpected remaining arguments
	if (optind < argc && argv[optind][0] != '-') {
		fprintf(stderr, "Unexpected argument: %s\n\n", argv[optind]);
		print_help(argv[0]);
		return 1;
	}

	// Load configuration file
	load_result = config_load_file(config_path);
	if (load_result.error_code != CONFIG_OK) {
		fprintf(stderr, "failed to load json config: %s (%s)\n",
			config_path, config_error_to_string(load_result.error_code));
		return load_result.error_code;
	}

	// Initialize logging system
	log_ret = log_init(load_result.config.zlog_config_path);
	if (log_ret != LOG_OK) {
		config_free(&load_result.config);
		fprintf(stderr, "failed to initialize zlog from: %s (%s)\n",
			load_result.config.zlog_config_path,
			log_error_to_string(log_ret));
		return 1;
	}

	// Log version and build information
	LOG_INFO("version: %s", VERSION);
	LOG_INFO("build_time(UTC): %s", BUILD_TIME);

	// Run main application logic
	app_run(&load_result.config);

	// Shut down logging system and release resources
	log_shutdown();
	config_free(&load_result.config);
	
	return 0;
}
