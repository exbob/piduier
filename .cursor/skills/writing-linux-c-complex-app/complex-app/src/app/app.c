#define _DEFAULT_SOURCE

#include "app.h"

#include "log.h"

#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t g_stop;
static volatile sig_atomic_t g_last_signal;

static const char *signal_name(int signo)
{
	switch (signo) {
	case SIGINT:
		return "SIGINT";
	case SIGQUIT:
		return "SIGQUIT";
	default:
		return "UNKNOWN";
	}
}

static void on_sigint(int signum)
{
	g_last_signal = signum;
	g_stop = 1;
}

void app_run(const struct json_config *cfg)
{
	struct sigaction sa;
	struct sigaction old_int;
	struct sigaction old_quit;

	if (!cfg) {
		return;
	}

	g_stop = 0;
	g_last_signal = 0;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_sigint;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, &old_int);
	sigaction(SIGQUIT, &sa, &old_quit);

	LOG_INFO("complex-app started; interval=%u s (Ctrl+C to exit)", cfg->interval_seconds);

	while (!g_stop) {
		LOG_INFO("heartbeat tick");
		sleep(cfg->interval_seconds);
	}
	if (g_last_signal != 0) {
		LOG_DEBUG("received signal: %s(%d)",
			signal_name((int)g_last_signal), (int)g_last_signal);
	}

	LOG_INFO("shutdown requested, exiting");
	sigaction(SIGINT, &old_int, NULL);
	sigaction(SIGQUIT, &old_quit, NULL);
}
