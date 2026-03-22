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
            "Usage: piduier [options]\n"
            "  -c, --zlog-conf PATH   zlog configuration file (default: ./zlog.conf)\n"
            "  -l, --log-file PATH    log file for Release file rules (default: ./piduier.log)\n"
            "  -h, --help             show this help\n"
            "\n"
            "Environment:\n"
            "  PIDUIER_ZLOG_CONF      same as --zlog-conf if no -c/--zlog-conf\n"
            "  PIDUIER_LOG_FILE       same as --log-file if no -l/--log-file\n");
}

static int parse_args(int argc, char **argv, const char **zlog_conf_out,
                      const char **log_file_out) {
    const char *zlog_conf = NULL;
    const char *log_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(stdout);
            return 2;
        }
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--zlog-conf") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "piduier: %s requires a path\n", argv[i]);
                return -1;
            }
            zlog_conf = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log-file") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "piduier: %s requires a path\n", argv[i]);
                return -1;
            }
            log_file = argv[++i];
            continue;
        }
        fprintf(stderr, "piduier: unknown option: %s\n", argv[i]);
        print_usage(stderr);
        return -1;
    }

    if (zlog_conf == NULL || zlog_conf[0] == '\0') {
        const char *e = getenv("PIDUIER_ZLOG_CONF");
        if (e != NULL && e[0] != '\0') {
            zlog_conf = e;
        } else {
            zlog_conf = "./zlog.conf";
        }
    }

    if (log_file == NULL || log_file[0] == '\0') {
        const char *e = getenv("PIDUIER_LOG_FILE");
        if (e != NULL && e[0] != '\0') {
            log_file = e;
        } else {
            log_file = "./piduier.log";
        }
    }

    if (setenv("PIDUIER_LOG_FILE", log_file, 1) != 0) {
        fprintf(stderr, "piduier: setenv(PIDUIER_LOG_FILE) failed\n");
        return -1;
    }

    *zlog_conf_out = zlog_conf;
    *log_file_out = log_file;
    return 0;
}

int main(int argc, char **argv)
{
    const char *zlog_conf;
    const char *log_file;
    int prc = parse_args(argc, argv, &zlog_conf, &log_file);
    if (prc < 0) {
        return EXIT_FAILURE;
    }
    if (prc == 2) {
        return EXIT_SUCCESS;
    }

	if (log_init(zlog_conf) != 0) {
		return EXIT_FAILURE;
	}
	atexit(log_fini);

	LOG_INFO("piduier starting version=%s arch=%s zlog_conf=%s log_file=%s",
	         PIDUIER_VERSION, PIDUIER_BUILD_ARCH, zlog_conf, log_file);

#ifdef PIDUIER_DEBUG_LOG
	mg_log_set(MG_LL_DEBUG);
#else
	mg_log_set(MG_LL_INFO);
#endif

	struct mg_mgr mgr; // Mongoose event manager. Holds all connections
	mg_mgr_init(&mgr); // Initialise event manager
	LOG_INFO("mongoose event manager initialized");

	// 启动 CPU 监控后台线程
	pthread_t cpu_thread;
	pthread_create(&cpu_thread, NULL, cpu_monitor_thread, NULL);
	pthread_detach(cpu_thread);
	LOG_INFO("CPU monitor thread started");

	mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);
	LOG_INFO("HTTP server listening on http://0.0.0.0:8000");
	// HTTPS 暂时禁用，需要证书
	// mg_http_listen(&mgr, "https://0.0.0.0:8443", ev_handler, NULL);
	
	for (;;) {
		mg_mgr_poll(&mgr, 100); // Infinite event loop
        router_broadcast_gpio_if_changed(&mgr);
	}
	return EXIT_SUCCESS;
}
