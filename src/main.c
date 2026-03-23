#include "config/app_config.h"
#include "mongoose.h"
#include "http/router.h"
#include "system/cpu.h"
#include "util/log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PIDUIER_VERSION
#define PIDUIER_VERSION "unknown"
#endif
#ifndef PIDUIER_BUILD_ARCH
#define PIDUIER_BUILD_ARCH "unknown"
#endif

static piduier_config_t g_cfg;

static void piduier_exit_cleanup(void) {
	log_fini();
	piduier_config_free(&g_cfg);
}

// Connection event handler function
static void ev_handler(struct mg_connection *c, int ev, void *ev_data)
{
	if (ev == MG_EV_HTTP_MSG) { // New HTTP request received
		struct mg_http_message *hm = (struct mg_http_message *)ev_data;
		router_handle_request(c, hm);
	}
}

// 后台线程：定期更新 CPU 占用率缓存
static void *cpu_monitor_thread(void *arg) {
    (void)arg;
    
    // 初始化：第一次采样
    cpu_time_t prev, curr;
    if (cpu_read_time(&prev) == 0) {
        sleep(1);
        if (cpu_read_time(&curr) == 0) {
            // 计算初始占用率
            double usage = cpu_calculate_usage(&prev, &curr);
            (void)usage; // 暂时不使用
        }
    }
    
    // 定期更新（每 1 秒）
    while (1) {
        sleep(1);
        cpu_update_cache();
    }
    
    return NULL;
}

static void print_usage(FILE *fp) {
    fprintf(fp,
            "piduier %s\n"
            "PiDuier is a Raspberry Pi 5 40-pin interface debugging platform for embedded software and hardware development.\n"
            "Usage: piduier [options]\n"
            "  -f, --config PATH   application config (JSON), default: ./piduier.conf\n"
            "  -h, --help          show this help\n",
            PIDUIER_VERSION);
}

static int parse_args(int argc, char **argv, const char **config_path_out) {
    const char *config_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(stdout);
            return 2;
        }
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "piduier: %s requires a path\n", argv[i]);
                return -1;
            }
            config_path = argv[++i];
            continue;
        }
        fprintf(stderr, "piduier: unknown option: %s\n", argv[i]);
        print_usage(stderr);
        return -1;
    }

    if (config_path == NULL || config_path[0] == '\0') {
        config_path = "./piduier.conf";
    }

    *config_path_out = config_path;
    return 0;
}

int main(int argc, char **argv)
{
    const char *config_path;
    int prc = parse_args(argc, argv, &config_path);
    if (prc < 0) {
        return EXIT_FAILURE;
    }
    if (prc == 2) {
        return EXIT_SUCCESS;
    }

	char err[512];
	if (piduier_config_load(config_path, &g_cfg, err, sizeof(err)) != 0) {
		fprintf(stderr, "piduier: config error: %s\n", err);
		return EXIT_FAILURE;
	}

	if (log_init(g_cfg.zlog_ini) != 0) {
		piduier_config_free(&g_cfg);
		return EXIT_FAILURE;
	}
	atexit(piduier_exit_cleanup);

	LOG_INFO("piduier starting product=\"Raspberry Pi 5 40-pin interface debugging platform\" version=%s arch=%s config=%s http_listen=%s http_port=%d log_file=%s",
	         PIDUIER_VERSION, PIDUIER_BUILD_ARCH, config_path,
	         g_cfg.http_listen, g_cfg.http_port, g_cfg.log_file);
    router_set_web_root(g_cfg.web_root);
    LOG_INFO("web root set to %s", g_cfg.web_root);

#ifdef PIDUIER_DEBUG_LOG
	mg_log_set(MG_LL_DEBUG);
#else
	mg_log_set(MG_LL_INFO);
#endif

	char listen_url[320];
	if (snprintf(listen_url, sizeof(listen_url), "http://%s:%d",
	             g_cfg.http_listen, g_cfg.http_port) >= (int)sizeof(listen_url)) {
		LOG_ERROR("listen URL too long");
		return EXIT_FAILURE;
	}

	struct mg_mgr mgr; // Mongoose event manager. Holds all connections
	mg_mgr_init(&mgr); // Initialise event manager
	LOG_INFO("mongoose event manager initialized");

	// 启动 CPU 监控后台线程
	pthread_t cpu_thread;
	pthread_create(&cpu_thread, NULL, cpu_monitor_thread, NULL);
	pthread_detach(cpu_thread);
	LOG_INFO("CPU monitor thread started");

	mg_http_listen(&mgr, listen_url, ev_handler, NULL);
	LOG_INFO("HTTP server listening on %s", listen_url);
	// HTTPS 暂时禁用，需要证书
	// mg_http_listen(&mgr, "https://0.0.0.0:8443", ev_handler, NULL);
	
	for (;;) {
		mg_mgr_poll(&mgr, 100); // Infinite event loop
        router_broadcast_gpio_if_changed(&mgr);
	}
	return EXIT_SUCCESS;
}
