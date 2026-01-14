#include "router.h"
#include "../system/system_info.h"
#include "../system/datetime.h"
#include "../system/cpu.h"
#include "../system/memory.h"
#include "../system/uptime.h"
#include "../system/temperature.h"
#include "../system/storage.h"
#include "../network/network_manager.h"
#include "../network/ethernet.h"
#include "../network/wifi.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

static void handle_ethernet_config(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        const char *device = "eth0";
        char device_buf[16] = {0};
        if (mg_http_get_var(&hm->query, "device", device_buf, sizeof(device_buf)) > 0) {
            device = device_buf;
        }
        
        ethernet_config_t config;
        if (ethernet_get_config(device, &config) == 0) {
            char *json = ethernet_config_to_json(&config);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to get ethernet config\"}\n");
        }
    } else if (mg_match(hm->method, mg_str("POST"), NULL)) {
        const char *device = "eth0";
        char *device_param = mg_json_get_str(hm->body, "$.device");
        if (device_param != NULL && strlen(device_param) > 0) {
            device = device_param;
        }
        
        char *mode = mg_json_get_str(hm->body, "$.mode");
        
        int result = -1;
        if (mode != NULL && strcmp(mode, "dhcp") == 0) {
            result = ethernet_set_dhcp(device, NULL);
        } else if (mode != NULL && strcmp(mode, "static") == 0) {
            char *ip = mg_json_get_str(hm->body, "$.ip");
            char *netmask = mg_json_get_str(hm->body, "$.netmask");
            char *gateway = mg_json_get_str(hm->body, "$.gateway");
            char *dns1 = mg_json_get_str(hm->body, "$.dns[0]");
            char *dns2 = mg_json_get_str(hm->body, "$.dns[1]");
            
            if (ip != NULL) {
                result = ethernet_set_static(device, NULL, ip, netmask, gateway, dns1, dns2);
            }
            
            if (ip) free(ip);
            if (netmask) free(netmask);
            if (gateway) free(gateway);
            if (dns1) free(dns1);
            if (dns2) free(dns2);
        }
        
        if (result == 0) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"status\":\"success\"}\n");
        } else {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Failed to configure ethernet\"}\n");
        }
        
        if (device_param) free(device_param);
        if (mode) free(mode);
    }
}

static void handle_wifi_scan(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("GET"), NULL)) {
        wifi_network_list_t list;
        wifi_network_list_init(&list);
        
        if (wifi_scan(&list) == 0) {
            char *json = wifi_network_list_to_json(&list);
            if (json != NULL) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json);
                free(json);
            } else {
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\":\"Failed to generate JSON\"}\n");
            }
        } else {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"Failed to scan WiFi\"}\n");
        }
        
        wifi_network_list_free(&list);
    }
}

static void handle_wifi_connect(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("POST"), NULL)) {
        char *ssid = mg_json_get_str(hm->body, "$.ssid");
        char *password = mg_json_get_str(hm->body, "$.password");
        
        if (ssid != NULL && strlen(ssid) > 0) {
            int result = wifi_connect(ssid, password);
            if (result == 0) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                    "{\"status\":\"success\"}\n");
            } else {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                    "{\"status\":\"error\",\"message\":\"Failed to connect WiFi\"}\n");
            }
        } else {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Missing SSID\"}\n");
        }
        
        if (ssid) free(ssid);
        if (password) free(password);
    }
}

static void handle_wifi_disconnect(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_match(hm->method, mg_str("POST"), NULL)) {
        char *device = mg_json_get_str(hm->body, "$.device");
        
        int result = wifi_disconnect(device);
        if (result == 0) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"status\":\"success\"}\n");
        } else {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                "{\"status\":\"error\",\"message\":\"Failed to disconnect WiFi\"}\n");
        }
        
        if (device) free(device);
    }
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
    // 网络设备 API
    else if (mg_match(hm->uri, mg_str("/api/network/devices"), NULL)) {
        handle_network_devices(c, hm);
    }
    // 以太网配置 API
    else if (mg_match(hm->uri, mg_str("/api/network/ethernet/config"), NULL)) {
        handle_ethernet_config(c, hm);
    }
    // Wi-Fi 扫描 API
    else if (mg_match(hm->uri, mg_str("/api/network/wifi/scan"), NULL)) {
        handle_wifi_scan(c, hm);
    }
    // Wi-Fi 连接 API
    else if (mg_match(hm->uri, mg_str("/api/network/wifi/connect"), NULL)) {
        handle_wifi_connect(c, hm);
    }
    // Wi-Fi 断开 API
    else if (mg_match(hm->uri, mg_str("/api/network/wifi/disconnect"), NULL)) {
        handle_wifi_disconnect(c, hm);
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
        struct mg_http_serve_opts opts = {.root_dir = "web", .fs = &mg_fs_posix};
        mg_http_serve_dir(c, hm, &opts);
    }
}
