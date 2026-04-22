#include "log.h"
#include "Config.h"

#include <getopt.h>
#include <stdio.h>

static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{0, 0, 0, 0}
};

static void print_help(const char *prog)
{
	printf("Usage: %s [OPTION]\n", prog);
	printf("A tiny Linux C demo application.\n\n");
	printf("Options:\n");
	printf("  -h, --help     Show this help message\n");
	printf("  -v, --version  Show program version information\n");
}

static void print_version(void)
{
	printf("Program: small-app\n");
	printf("Description: A tiny Linux C demo application.\n");
	printf("Version: %s\n", VERSION);
	printf("Build time(UTC): %s\n", BUILD_TIME);
}

int main(int argc, char *argv[])
{
	int c;

	opterr = 0;

	while ((c = getopt_long(argc, argv, "hv", long_options, NULL)) != -1) {
		switch (c) {
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

	if (optind < argc) {
		fprintf(stdout, "Unexpected argument: %s\n\n", argv[optind]);
		print_help(argv[0]);
		return 1;
	}

	LOG_INFO("small-app start");
	LOG_INFO("version: %s", VERSION);
	LOG_INFO("build_time(UTC): %s", BUILD_TIME);
	return 0;
}
