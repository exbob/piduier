#include "router.h"
#include "Config.h"
#include "../system/system_info.h"
#include "../system/datetime.h"
#include "../system/cpu.h"
#include "../system/memory.h"
#include "../system/uptime.h"
#include "../system/temperature.h"
#include "../system/storage.h"
#include "../network/network_manager.h"
#include "../hardware/gpio.h"
#include "../util/log.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef PIDUIER_VERSION
#define PIDUIER_VERSION "unknown"
#endif

#ifndef PIDUIER_BUILD_ARCH
#define PIDUIER_BUILD_ARCH "unknown"
#endif

#define GPIO_HINT "Check pinctrl availability, pin ownership, and permissions."
static char *last_gpio_ws_json = NULL;
static uint64_t last_gpio_ws_check_ms = 0;
static char g_web_root[1024] = "./web";

void router_set_web_root(const char *web_root) {
    if (web_root == NULL || web_root[0] == '\0') {
        return;
    }
    snprintf(g_web_root, sizeof(g_web_root), "%s", web_root);
}

static void handle_system_version(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"version\":\"%s\",\"build_arch\":\"%s\"}\n",
            PIDUIER_VERSION, PIDUIER_BUILD_ARCH);
    } else {
        mg_http_reply(c, 405, "Content-Type: application/json\r\n",
            "{\"error\":\"Method not allowed\"}\n");
    }
}

static void handle_system_info(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        system_info_t info;
        if (system_get_info(&info) == 0) {
            char *json = system_info_to_json(&info);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get system info\"}\n");
        }
    }
}

static void handle_hostname(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        system_info_t info;
        if (system_get_info(&info) == 0) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"static_hostname\":\"%s\",\"pretty_hostname\":\"%s\"}\n",
                info.static_hostname, info.pretty_hostname);
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get hostname\"}\n");
        }
    } else if (mg_match(hm->method, mg_str("POST"), NULL)) {
        char *hostname = mg_json_get_str(hm->body, "$.hostname");
        if (hostname != NULL && strlen(hostname) > 0) {
            int result = system_set_hostname(hostname);
            if (result == 0) {
                LOG_INFO("[system] hostname set to %s", hostname);
                mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                    "{\"status\":\"success\"}\n");
            } else {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                    "{\"status\":\"error\",\"message\":\"Invalid hostname or failed to set\"}\n");
            }
            free(hostname);
        } else {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Missing or invalid hostname in request\"}\n");
        }
    }
}

static void handle_datetime(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        datetime_info_t info;
        if (datetime_get_info(&info) == 0) {
            char *json = datetime_info_to_json(&info);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get datetime info\"}\n");
        }
    } else if (mg_match(hm->method, mg_str("POST"), NULL)) {
        char *datetime = mg_json_get_str(hm->body, "$.datetime");
        if (datetime != NULL && strlen(datetime) > 0) {
            int result = datetime_set_time(datetime);
            if (result == 0) {
                LOG_INFO("[system] datetime set to %s", datetime);
                mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                    "{\"status\":\"success\"}\n");
            } else {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                    "{\"status\":\"error\",\"message\":\"Failed to set datetime\"}\n");
            }
            free(datetime);
        } else {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Missing datetime in request\"}\n");
        }
    }
}

static void handle_timezone(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        size_t count;
        char **timezones = datetime_get_timezones(&count);
        if (timezones != NULL) {
            size_t json_size = 1024 * count;
            char *json = malloc(json_size);
            if (json != NULL) {
                strcpy(json, "[");
                char *p = json + 1;
                for (size_t i = 0; i < count; i++) {
                    int written = snprintf(p, json_size - (p - json),
                        "%s\"%s\"", (i > 0) ? "," : "", timezones[i]);
                    if (written > 0) {
                        p += written;
                    }
                }
                strcpy(p, "]");
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            }
            datetime_free_timezones(timezones);
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get timezones\"}\n");
        }
    } else if (mg_match(hm->method, mg_str("POST"), NULL)) {
        char *timezone = mg_json_get_str(hm->body, "$.timezone");
        if (timezone != NULL && strlen(timezone) > 0) {
            int result = datetime_set_timezone(timezone);
            if (result == 0) {
                LOG_INFO("[system] timezone set to %s", timezone);
                mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                    "{\"status\":\"success\"}\n");
            } else {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                    "{\"status\":\"error\",\"message\":\"Failed to set timezone\"}\n");
            }
            free(timezone);
        } else {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Missing timezone in request\"}\n");
        }
    }
}

static void handle_cpu(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        double usage = cpu_get_usage();
        if (usage >= 0) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"usage\":%.2f}\n", usage);
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get CPU usage\"}\n");
        }
    }
}

static void handle_memory(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        memory_info_t info;
        if (memory_get_info(&info) == 0) {
            char *json = memory_info_to_json(&info);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get memory info\"}\n");
        }
    }
}

static void handle_uptime(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        long seconds = uptime_get_seconds();
        if (seconds >= 0) {
            char formatted[128];
            uptime_format(seconds, formatted, sizeof(formatted));
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"uptime\":%ld,\"uptime_formatted\":\"%s\"}\n", seconds, formatted);
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get uptime\"}\n");
        }
    }
}

static void handle_temperature(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        double temp = temperature_get_cpu();
        if (temp >= 0) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"temperature\":%.2f,\"unit\":\"celsius\"}\n", temp);
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get CPU temperature\"}\n");
        }
    }
}

// 处理存储信息 API
static void handle_storage(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        // GET /api/system/storage
        storage_info_t info;
        if (storage_get_info("/dev/mmcblk0p2", &info) == 0) {
            char *json = storage_info_to_json(&info);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get storage info\"}\n");
        }
    }
}

static void handle_network_devices(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        network_device_list_t list;
        network_device_list_init(&list);
        
        if (network_get_devices(&list) == 0) {
            char *json = network_device_list_to_json(&list);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get network devices\"}\n");
        }
        
        network_device_list_free(&list);
    }
}

static void handle_gpio_status(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        gpio_status_t status;
        gpio_status_init(&status);
        
        if (gpio_get_all_status(&status) == 0) {
            char *json = gpio_status_to_json(&status);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 503, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get GPIO status\",\"hint\":\"%s\"}\n", GPIO_HINT);
        }
        
        gpio_status_free(&status);
    }
}

static void handle_gpio_pin(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        // 从 URI 中提取 GPIO 编号，例如 /api/gpio/17
        struct mg_str uri = hm->uri;
        int gpio_num = -1;
        
        // 查找最后一个 / 后的数字
        const char *p = (const char *)uri.buf + uri.len - 1;
        while (p >= (const char *)uri.buf && *p != '/') p--;
        if (p >= (const char *)uri.buf) {
            p++;
            gpio_num = atoi(p);
        }
        
        if (gpio_num < 0 || !gpio_is_supported(gpio_num)) {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"error\":\"Invalid or unsupported GPIO pin\"}\n");
            return;
        }
        
        gpio_pin_info_t info;
        if (gpio_get_pin_status(gpio_num, &info) == 0) {
            char *json = gpio_pin_info_to_json(&info);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 503, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get GPIO pin status\",\"hint\":\"%s\"}\n", GPIO_HINT);
        }
    }
}

static int parse_gpio_num_from_uri(const struct mg_str uri) {
    char uri_buf[128];
    int gpio_num = -1;
    if (uri.len <= 0 || uri.len >= (int) sizeof(uri_buf)) {
        return -1;
    }
    snprintf(uri_buf, sizeof(uri_buf), "%.*s", (int) uri.len, uri.buf);

    if (sscanf(uri_buf, "/api/gpio/%d/mode", &gpio_num) == 1) {
        LOG_DEBUG("[GPIO][router] parse uri='%s' -> gpio=%d (mode)", uri_buf, gpio_num);
        return gpio_num;
    }
    if (sscanf(uri_buf, "/api/gpio/%d/value", &gpio_num) == 1) {
        LOG_DEBUG("[GPIO][router] parse uri='%s' -> gpio=%d (value)", uri_buf, gpio_num);
        return gpio_num;
    }
    if (sscanf(uri_buf, "/api/gpio/%d", &gpio_num) == 1) {
        LOG_DEBUG("[GPIO][router] parse uri='%s' -> gpio=%d (pin)", uri_buf, gpio_num);
        return gpio_num;
    }
    LOG_DEBUG("[GPIO][router] failed to parse uri='%s'", uri_buf);
    return -1;
}

static int uri_has_suffix(struct mg_str uri, const char *suffix) {
    size_t suffix_len = strlen(suffix);
    if (uri.len < suffix_len) {
        return 0;
    }
    return memcmp(uri.buf + uri.len - suffix_len, suffix, suffix_len) == 0;
}

static int uri_has_prefix(struct mg_str uri, const char *prefix) {
    size_t prefix_len = strlen(prefix);
    if (uri.len < prefix_len) {
        return 0;
    }
    return memcmp(uri.buf, prefix, prefix_len) == 0;
}

static int has_gpio_ws_clients(struct mg_mgr *mgr) {
    for (struct mg_connection *it = mgr->conns; it != NULL; it = it->next) {
        if (it->is_websocket && it->data[0] == 1 && !it->is_closing) {
            return 1;
        }
    }
    return 0;
}

static void handle_gpio_mode(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("POST"), NULL)) {
        int gpio_num = parse_gpio_num_from_uri(hm->uri);
        
        if (gpio_num < 0 || !gpio_is_supported(gpio_num)) {
            LOG_INFO("[GPIO][router] mode request rejected: gpio=%d supported=%d",
                gpio_num, gpio_is_supported(gpio_num));
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"error\":\"Invalid or unsupported GPIO pin\"}\n");
            return;
        }
        
        char *mode = mg_json_get_str(hm->body, "$.mode");
        int result = -1;
        int initial_value = 0;

        if (mode != NULL) {
            if (strcmp(mode, "input") == 0) {
                result = gpio_set_input(gpio_num);
            } else if (strcmp(mode, "output") == 0) {
                initial_value = (int) mg_json_get_long(hm->body, "$.initial_value", 0);
                if (initial_value != 0 && initial_value != 1) {
                    initial_value = 0;
                }
                result = gpio_set_output(gpio_num, initial_value);
            }
        }

        if (result == 0) {
            if (mode != NULL && strcmp(mode, "input") == 0) {
                LOG_INFO("[GPIO] gpio=%d configured as input", gpio_num);
            } else if (mode != NULL && strcmp(mode, "output") == 0) {
                LOG_INFO("[GPIO] gpio=%d configured as output initial=%d", gpio_num, initial_value);
            } else if (mode != NULL) {
                LOG_INFO("[GPIO] gpio=%d mode=%s applied", gpio_num, mode);
            }
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"status\":\"success\"}\n");
        } else {
            LOG_ERROR("[GPIO][router] mode set failed gpio=%d mode=%s result=%d errno=%d",
                gpio_num, mode ? mode : "(null)", result, errno);
            mg_http_reply(c, 503, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Failed to set GPIO mode\",\"hint\":\"%s\"}\n", GPIO_HINT);
        }
        
        if (mode) free(mode);
    }
}

static void handle_gpio_value(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        int gpio_num = parse_gpio_num_from_uri(hm->uri);
        
        if (gpio_num < 0 || !gpio_is_supported(gpio_num)) {
            LOG_INFO("[GPIO][router] value request rejected: gpio=%d supported=%d",
                gpio_num, gpio_is_supported(gpio_num));
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"error\":\"Invalid or unsupported GPIO pin\"}\n");
            return;
        }
        
        int value = gpio_get_value(gpio_num);
        if (value >= 0) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"gpio_num\":%d,\"value\":%d}\n", gpio_num, value);
        } else {
            mg_http_reply(c, 503, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get GPIO value\",\"hint\":\"%s\"}\n", GPIO_HINT);
        }
    } else if (mg_match(hm->method, mg_str("POST"), NULL)) {
        int gpio_num = parse_gpio_num_from_uri(hm->uri);
        
        if (gpio_num < 0 || !gpio_is_supported(gpio_num)) {
            LOG_INFO("[GPIO][router] value POST rejected: gpio=%d supported=%d",
                gpio_num, gpio_is_supported(gpio_num));
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"error\":\"Invalid or unsupported GPIO pin\"}\n");
            return;
        }
        
        int value = (int) mg_json_get_long(hm->body, "$.value", -1);
        
        if (value < 0 || value > 1) {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"error\":\"Invalid value, must be 0 or 1\"}\n");
            return;
        }
        
        int result = gpio_set_value(gpio_num, value);
        if (result == 0) {
            LOG_INFO("[GPIO] gpio=%d output value set to %d", gpio_num, value);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"status\":\"success\"}\n");
        } else {
            LOG_ERROR("[GPIO][router] set value failed gpio=%d value=%d result=%d errno=%d",
                gpio_num, value, result, errno);
            mg_http_reply(c, 503, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Failed to set GPIO value\",\"hint\":\"%s\"}\n", GPIO_HINT);
        }
    }
}

static const char *pull_to_str(gpio_pull_t pull) {
    switch (pull) {
        case GPIO_PULL_UP: return "up";
        case GPIO_PULL_DOWN: return "down";
        case GPIO_PULL_OFF: return "off";
        default: return "unknown";
    }
}

static const char *drive_to_str(int drive) {
    if (drive == 1) return "dh";
    if (drive == 0) return "dl";
    return "unknown";
}

static void handle_gpio_attrs(struct mg_connection *c, struct mg_http_message *hm) {
    int gpio_num = parse_gpio_num_from_uri(hm->uri);
    if (gpio_num < 0 || !gpio_is_supported(gpio_num)) {
        LOG_INFO("[GPIO] attrs request rejected: gpio=%d supported=%d",
            gpio_num, gpio_is_supported(gpio_num));
        mg_http_reply(c, 400, "Content-Type: application/json\r\n",
            "{\"error\":\"Invalid or unsupported GPIO pin\"}\n");
        return;
    }

    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        gpio_pin_attrs_t attrs;
        if (gpio_get_attrs(gpio_num, &attrs) == 0) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"gpio_num\":%d,\"pull\":\"%s\",\"drive\":\"%s\",\"available\":%d}\n",
                attrs.gpio_num, pull_to_str(attrs.pull), drive_to_str(attrs.drive), attrs.available);
        } else {
            mg_http_reply(c, 503, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get GPIO attrs\",\"hint\":\"%s\"}\n", GPIO_HINT);
        }
        return;
    }

    if (mg_match(hm->method, mg_str("POST"), NULL)) {
        char *pull = mg_json_get_str(hm->body, "$.pull");
        char *drive = mg_json_get_str(hm->body, "$.drive");
        int has_pull = (pull != NULL);
        int has_drive = (drive != NULL);

        if (!has_pull && !has_drive) {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"error\":\"Nothing to update. Provide pull and/or drive.\"}\n");
            if (pull) free(pull);
            if (drive) free(drive);
            return;
        }

        if (has_pull && gpio_set_pull(gpio_num, pull) != 0) {
            LOG_ERROR("[GPIO] gpio=%d failed to set pull=%s", gpio_num, pull);
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Failed to set pull\"}\n");
            if (pull) free(pull);
            if (drive) free(drive);
            return;
        }
        if (has_drive && gpio_set_drive(gpio_num, drive) != 0) {
            LOG_ERROR("[GPIO] gpio=%d failed to set drive=%s", gpio_num, drive);
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Failed to set drive mode\"}\n");
            if (pull) free(pull);
            if (drive) free(drive);
            return;
        }

        LOG_INFO("[GPIO] gpio=%d attrs updated pull=%s drive=%s",
            gpio_num,
            has_pull ? pull : "(unchanged)",
            has_drive ? drive : "(unchanged)");

        if (pull) free(pull);
        if (drive) free(drive);
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"status\":\"success\"}\n");
        return;
    }

    mg_http_reply(c, 405, "Content-Type: application/json\r\n",
        "{\"error\":\"Method not allowed\"}\n");
}

static void handle_system_info_complete(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        // 构建完整的系统信息 JSON
        
        // 获取系统信息
        system_info_t sys_info;
        int sys_ok = (system_get_info(&sys_info) == 0);
        char *sys_json = sys_ok ? system_info_to_json(&sys_info) : NULL;
        
        // 获取日期时间
        datetime_info_t dt_info;
        int dt_ok = (datetime_get_info(&dt_info) == 0);
        char *dt_json = dt_ok ? datetime_info_to_json(&dt_info) : NULL;
        
        // 获取 CPU 占用率
        double cpu_usage = cpu_get_usage();
        
        // 获取内存信息
        memory_info_t mem_info;
        int mem_ok = (memory_get_info(&mem_info) == 0);
        char *mem_json = mem_ok ? memory_info_to_json(&mem_info) : NULL;
        
        // 获取运行时间
        long uptime_sec = uptime_get_seconds();
        char uptime_str[128] = "-";
        if (uptime_sec >= 0) {
            uptime_format(uptime_sec, uptime_str, sizeof(uptime_str));
        }
        
        // 获取网络设备
        network_device_list_t net_list;
        network_device_list_init(&net_list);
        int net_ok = (network_get_devices(&net_list) == 0);
        char *net_json = net_ok ? network_device_list_to_json(&net_list) : NULL;
        
        // 获取 CPU 温度
        double temperature = temperature_get_cpu();
        
        // 获取存储信息
        storage_info_t storage_info;
        int storage_ok = (storage_get_info("/dev/mmcblk0p2", &storage_info) == 0);
        char *storage_json = storage_ok ? storage_info_to_json(&storage_info) : NULL;
        
        // 构建 JSON 响应
        size_t json_size = 6144; // 增加缓冲区大小以容纳新数据
        char *json = malloc(json_size);
        if (json != NULL) {
            int len = snprintf(json, json_size,
                "{"
                "\"system\":%s,"
                "\"datetime\":%s,"
                "\"cpu_usage\":%.2f,"
                "\"memory\":%s,"
                "\"uptime\":%ld,"
                "\"uptime_formatted\":\"%s\","
                "\"temperature\":%.2f,"
                "\"storage\":%s,"
                "\"network\":%s"
                "}",
                sys_json ? sys_json : "null",
                dt_json ? dt_json : "null",
                cpu_usage >= 0 ? cpu_usage : 0.0,
                mem_json ? mem_json : "null",
                uptime_sec >= 0 ? uptime_sec : 0,
                uptime_str,
                temperature >= 0 ? temperature : 0.0,
                storage_json ? storage_json : "null",
                net_json ? net_json : "[]"
            );
            
            // 检查缓冲区是否足够
            if (len >= (int)json_size) {
                // 缓冲区不够，重新分配更大的缓冲区
                free(json);
                json_size = len + 1024;
                json = malloc(json_size);
                if (json != NULL) {
                    snprintf(json, json_size,
                        "{"
                        "\"system\":%s,"
                        "\"datetime\":%s,"
                        "\"cpu_usage\":%.2f,"
                        "\"memory\":%s,"
                        "\"uptime\":%ld,"
                        "\"uptime_formatted\":\"%s\","
                        "\"temperature\":%.2f,"
                        "\"storage\":%s,"
                        "\"network\":%s"
                        "}",
                        sys_json ? sys_json : "null",
                        dt_json ? dt_json : "null",
                        cpu_usage >= 0 ? cpu_usage : 0.0,
                        mem_json ? mem_json : "null",
                        uptime_sec >= 0 ? uptime_sec : 0,
                        uptime_str,
                        temperature >= 0 ? temperature : 0.0,
                        storage_json ? storage_json : "null",
                        net_json ? net_json : "[]"
                    );
                }
            }
            
            if (json != NULL) {
                // 释放临时 JSON 字符串
                if (sys_json) free(sys_json);
                if (dt_json) free(dt_json);
                if (mem_json) free(mem_json);
                if (net_json) free(net_json);
                if (storage_json) free(storage_json);
                
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                // 释放已分配的资源
                if (sys_json) free(sys_json);
                if (dt_json) free(dt_json);
                if (mem_json) free(mem_json);
                if (net_json) free(net_json);
                if (storage_json) free(storage_json);
                
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to allocate memory\"}\n");
            }
        } else {
            // 释放已分配的资源
            if (sys_json) free(sys_json);
            if (dt_json) free(dt_json);
            if (mem_json) free(mem_json);
            if (net_json) free(net_json);
            if (storage_json) free(storage_json);
            
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to allocate memory\"}\n");
        }
        
        network_device_list_free(&net_list);
    }
}

static void handle_reboot(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("POST"), NULL)) {
        // POST /api/system/reboot
        // 执行重启命令（需要 root 权限）
        LOG_INFO("[system] reboot requested via API");
        int result = system("sudo reboot");
        if (result == 0) {
            // 命令执行成功（虽然可能不会返回，因为系统会重启）
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"status\":\"success\",\"message\":\"System reboot initiated\"}\n");
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Failed to execute reboot command\"}\n");
        }
    } else {
        mg_http_reply(c, 405, "Content-Type: application/json\r\n",
            "{\"status\":\"error\",\"message\":\"Method not allowed\"}\n");
    }
}

static void handle_shutdown(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("POST"), NULL)) {
        // POST /api/system/shutdown
        // 执行关闭命令（需要 root 权限）
        LOG_INFO("[system] shutdown requested via API");
        int result = system("sudo shutdown -h now");
        if (result == 0) {
            // 命令执行成功（虽然可能不会返回，因为系统会关闭）
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"status\":\"success\",\"message\":\"System shutdown initiated\"}\n");
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Failed to execute shutdown command\"}\n");
        }
    } else {
        mg_http_reply(c, 405, "Content-Type: application/json\r\n",
            "{\"status\":\"error\",\"message\":\"Method not allowed\"}\n");
    }
}

void router_handle_request(struct mg_connection *c, struct mg_http_message *hm) {
    // GPIO WebSocket stream
    if (mg_match(hm->uri, mg_str("/ws/gpio"), NULL)) {
        mg_ws_upgrade(c, hm, NULL);
        c->data[0] = 1;  // mark as gpio stream client
        return;
    }

    // 系统信息 API
    if (mg_match(hm->uri, mg_str("/api/system/info"), NULL)) {
        handle_system_info(c, hm);
    }
    // 完整系统信息 API（包含所有监控数据）
    else if (mg_match(hm->uri, mg_str("/api/system/info/complete"), NULL)) {
        handle_system_info_complete(c, hm);
    }
    // 主机名 API
    else if (mg_match(hm->uri, mg_str("/api/system/hostname"), NULL)) {
        handle_hostname(c, hm);
    }
    // 日期时间 API
    else if (mg_match(hm->uri, mg_str("/api/system/datetime"), NULL)) {
        handle_datetime(c, hm);
    }
    // 时区 API
    else if (mg_match(hm->uri, mg_str("/api/system/timezone"), NULL) ||
             mg_match(hm->uri, mg_str("/api/system/timezones"), NULL)) {
        handle_timezone(c, hm);
    }
    // CPU 占用率 API
    else if (mg_match(hm->uri, mg_str("/api/system/cpu"), NULL)) {
        handle_cpu(c, hm);
    }
    // 内存使用率 API
    else if (mg_match(hm->uri, mg_str("/api/system/memory"), NULL)) {
        handle_memory(c, hm);
    }
    // 系统运行时间 API
    else if (mg_match(hm->uri, mg_str("/api/system/uptime"), NULL)) {
        handle_uptime(c, hm);
    }
    // CPU 温度 API
    else if (mg_match(hm->uri, mg_str("/api/system/temperature"), NULL)) {
        handle_temperature(c, hm);
    }
    // 存储信息 API
    else if (mg_match(hm->uri, mg_str("/api/system/storage"), NULL)) {
        handle_storage(c, hm);
    }
    // 系统版本 API
    else if (mg_match(hm->uri, mg_str("/api/system/version"), NULL)) {
        handle_system_version(c, hm);
    }
    // 网络设备 API
    else if (mg_match(hm->uri, mg_str("/api/network/devices"), NULL)) {
        handle_network_devices(c, hm);
    }
    // GPIO 状态 API
    else if (mg_match(hm->uri, mg_str("/api/gpio/status"), NULL)) {
        handle_gpio_status(c, hm);
    }
    // GPIO 模式 API (例如 /api/gpio/17/mode)
    else if (uri_has_prefix(hm->uri, "/api/gpio/") &&
             uri_has_suffix(hm->uri, "/mode")) {
        handle_gpio_mode(c, hm);
    }
    // GPIO 值 API (例如 /api/gpio/17/value)
    else if (uri_has_prefix(hm->uri, "/api/gpio/") &&
             uri_has_suffix(hm->uri, "/value")) {
        handle_gpio_value(c, hm);
    }
    // GPIO 属性 API (例如 /api/gpio/17/attrs)
    else if (uri_has_prefix(hm->uri, "/api/gpio/") &&
             uri_has_suffix(hm->uri, "/attrs")) {
        handle_gpio_attrs(c, hm);
    }
    // GPIO 单个引脚 API (例如 /api/gpio/17)
    else if (uri_has_prefix(hm->uri, "/api/gpio/")) {
        handle_gpio_pin(c, hm);
    }
    // 系统重启 API
    else if (mg_match(hm->uri, mg_str("/api/system/reboot"), NULL)) {
        handle_reboot(c, hm);
    }
    // 系统关闭 API
    else if (mg_match(hm->uri, mg_str("/api/system/shutdown"), NULL)) {
        handle_shutdown(c, hm);
    }
    // 测试 API
    else if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":1}\n");
    }
    // 静态文件服务
    else {
        struct mg_http_serve_opts opts = {.root_dir = g_web_root, .fs = &mg_fs_posix};
        mg_http_serve_dir(c, hm, &opts);
    }
}

void router_broadcast_gpio_if_changed(struct mg_mgr *mgr) {
    uint64_t now = mg_millis();
    if (now - last_gpio_ws_check_ms < 500) {
        return;
    }
    last_gpio_ws_check_ms = now;

    if (!has_gpio_ws_clients(mgr)) {
        return;
    }

    gpio_status_t status;
    gpio_status_init(&status);
    if (gpio_get_all_status(&status) != 0) {
        gpio_status_free(&status);
        return;
    }

    char *json = gpio_status_to_json(&status);
    gpio_status_free(&status);
    if (json == NULL) return;

    if (last_gpio_ws_json != NULL && strcmp(last_gpio_ws_json, json) == 0) {
        free(json);
        return;  // no change, skip push
    }

    free(last_gpio_ws_json);
    last_gpio_ws_json = json;

    for (struct mg_connection *it = mgr->conns; it != NULL; it = it->next) {
        if (it->is_websocket && it->data[0] == 1 && !it->is_closing) {
            mg_ws_send(it, last_gpio_ws_json, strlen(last_gpio_ws_json), WEBSOCKET_OP_TEXT);
        }
    }
}
