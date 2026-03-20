#include "mongoose.h"
#include "http/router.h"
#include "system/cpu.h"
#include <pthread.h>
#include <unistd.h>

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

int main()
{
	struct mg_mgr mgr; // Mongoose event manager. Holds all connections
	mg_mgr_init(&mgr); // Initialise event manager
	
	// 启动 CPU 监控后台线程
	pthread_t cpu_thread;
	pthread_create(&cpu_thread, NULL, cpu_monitor_thread, NULL);
	pthread_detach(cpu_thread);
	
	mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);
	// HTTPS 暂时禁用，需要证书
	// mg_http_listen(&mgr, "https://0.0.0.0:8443", ev_handler, NULL);
	
	for (;;) {
		mg_mgr_poll(&mgr, 100); // Infinite event loop
        router_broadcast_gpio_if_changed(&mgr);
	}
	return 0;
}
