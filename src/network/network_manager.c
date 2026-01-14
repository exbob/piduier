#include "network_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void network_device_list_init(network_device_list_t *list) {
    if (list == NULL) return;
    list->devices = NULL;
    list->count = 0;
    list->capacity = 0;
}

void network_device_list_free(network_device_list_t *list) {
    if (list == NULL) return;
    if (list->devices) {
        free(list->devices);
    }
    list->devices = NULL;
    list->count = 0;
    list->capacity = 0;
}

static int parse_device_status(const char *line, network_device_t *device) {
    if (line == NULL || device == NULL) {
        return -1;
    }
    
    if (strstr(line, "DEVICE") || strstr(line, "TYPE")) {
        return 0;
    }
    char device_name[16];
    char device_type[16];
    char device_state[32];
    char connection[64];
    
    if (sscanf(line, "%15s %15s %31s %63[^\n]", 
               device_name, device_type, device_state, connection) >= 3) {
        memset(device, 0, sizeof(network_device_t));
        // 使用 snprintf 避免警告
        snprintf(device->device, sizeof(device->device), "%s", device_name);
        snprintf(device->type, sizeof(device->type), "%s", device_type);
        snprintf(device->state, sizeof(device->state), "%s", device_state);
        if (connection[0]) {
            snprintf(device->connection, sizeof(device->connection), "%s", connection);
        }
        return 1;
    }
    
    return 0;
}

static void get_device_details(network_device_t *device) {
    if (device == NULL || device->device[0] == '\0') {
        return;
    }
    
    char cmd[256];
    char line[512];
    
    snprintf(cmd, sizeof(cmd), "nmcli device show %s 2>/dev/null", device->device);
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        return;
    }
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "IP4.ADDRESS[1]:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ') colon++;
                sscanf(colon, "%63[^/]", device->ip);
            }
        }
        else if (strstr(line, "GENERAL.HWADDR:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ') colon++;
                sscanf(colon, "%31s", device->mac);
            }
        }
        else if (strstr(line, "GENERAL.SPEED:") || strstr(line, "SPEED:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%15s", device->speed);
            }
        }
        else if (strcmp(device->type, "wifi") == 0) {
            if (strstr(line, "WIFI.SSID:") || strstr(line, "GENERAL.CONNECTION:")) {
                char *colon = strchr(line, ':');
                if (colon) {
                    colon++;
                    while (*colon == ' ' || *colon == '\t') colon++;
                    if (strstr(line, "WIFI.SSID:")) {
                        sscanf(colon, "%63[^\n]", device->ssid);
                    }
                }
            }
            else if (strstr(line, "WIFI.SIGNAL:") || strstr(line, "SIGNAL:")) {
                char *colon = strchr(line, ':');
                if (colon) {
                    colon++;
                    while (*colon == ' ' || *colon == '\t') colon++;
                    int signal;
                    if (sscanf(colon, "%d", &signal) == 1) {
                        device->signal = signal;
                    }
                }
            }
        }
    }
    
    pclose(fp);
    
    if (strcmp(device->type, "ethernet") == 0 && strlen(device->speed) == 0) {
        char speed_file[256];
        snprintf(speed_file, sizeof(speed_file), "/sys/class/net/%s/speed", device->device);
        FILE *f = fopen(speed_file, "r");
        if (f != NULL) {
            int speed_mbps;
            if (fscanf(f, "%d", &speed_mbps) == 1 && speed_mbps > 0) {
                snprintf(device->speed, sizeof(device->speed), "%d", speed_mbps);
            }
            fclose(f);
        }
    }
    
    if (strcmp(device->type, "wifi") == 0) {
        snprintf(cmd, sizeof(cmd), "nmcli device wifi list --rescan no 2>/dev/null | grep '^\\*' | head -1");
        fp = popen(cmd, "r");
        if (fp) {
            if (fgets(line, sizeof(line), fp) != NULL) {
                // 解析格式：*  BSSID  SSID  MODE  CHAN  RATE  SIGNAL  BARS  SECURITY
                // 使用 sscanf 按列解析
                char bssid[32] = {0};
                char ssid[64] = {0};
                char mode[16] = {0};
                char chan[16] = {0};
                char rate[32] = {0};
                int signal = 0;
                char bars[16] = {0};
                char security[32] = {0};
                
                // 跳过开头的 * 和空格
                char *p = line;
                while (*p == '*' || *p == ' ' || *p == '\t') p++;
                
                // 尝试解析：BSSID SSID MODE CHAN RATE SIGNAL BARS SECURITY
                // 注意：RATE 可能包含空格（如 "270 Mbit/s"），需要特殊处理
                if (sscanf(p, "%31s %63s %15s %15s %31s %d %15s %31s", 
                           bssid, ssid, mode, chan, rate, &signal, bars, security) >= 6) {
                    // 成功解析
                    if (strlen(device->ssid) == 0 && strlen(ssid) > 0) {
                        snprintf(device->ssid, sizeof(device->ssid), "%s", ssid);
                    }
                    if (device->signal == 0 && signal > 0 && signal <= 100) {
                        device->signal = signal;
                    }
                } else {
                    // 如果标准解析失败，尝试手动解析
                    // 跳过 BSSID
                    while (*p != ' ' && *p != '\t' && *p != '\0') p++;
                    while (*p == ' ' || *p == '\t') p++;
                    
                    // 提取 SSID（到下一个空格或制表符）
                    char *ssid_start = p;
                    char *ssid_end = p;
                    while (*ssid_end != ' ' && *ssid_end != '\t' && *ssid_end != '\n' && *ssid_end != '\0') {
                        ssid_end++;
                    }
                    size_t ssid_len = ssid_end - p;
                    if (ssid_len > 0 && ssid_len < sizeof(ssid)) {
                        memcpy(ssid, ssid_start, ssid_len);
                        ssid[ssid_len] = '\0';
                        if (strlen(device->ssid) == 0) {
                            snprintf(device->ssid, sizeof(device->ssid), "%s", ssid);
                        }
                    }
                    
                    // 查找信号强度：从后往前查找数字（通常在倒数第4列）
                    char *line_end = line + strlen(line) - 1;
                    while (line_end > line && (*line_end == '\n' || *line_end == ' ' || *line_end == '\t')) line_end--;
                    // 跳过最后几列（SECURITY, BARS），找到 SIGNAL
                    int spaces = 0;
                    char *num_start = line_end;
                    while (num_start > line && spaces < 2) {
                        if (*num_start == ' ' || *num_start == '\t') {
                            spaces++;
                            while (num_start > line && (*num_start == ' ' || *num_start == '\t')) num_start--;
                        } else {
                            num_start--;
                        }
                    }
                    // 现在 num_start 应该指向信号强度数字的开始
                    if (num_start > line) {
                        if (sscanf(num_start, "%d", &signal) == 1 && signal >= 0 && signal <= 100) {
                            device->signal = signal;
                        }
                    }
                }
            }
            pclose(fp);
        }
    }
}

int network_get_devices(network_device_list_t *list) {
    if (list == NULL) {
        return -1;
    }
    
    // 清空列表
    network_device_list_free(list);
    
    FILE *fp = popen("nmcli device status 2>/dev/null", "r");
    if (fp == NULL) {
        return -1;
    }
    
    char line[512];
    size_t capacity = 10;
    list->devices = malloc(capacity * sizeof(network_device_t));
    if (list->devices == NULL) {
        pclose(fp);
        return -1;
    }
    
    list->count = 0;
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        network_device_t device;
        if (parse_device_status(line, &device) > 0) {
            // 获取设备详细信息
            get_device_details(&device);
            
            // 扩展数组如果需要
            if (list->count >= capacity) {
                capacity *= 2;
                network_device_t *new_devices = realloc(list->devices, capacity * sizeof(network_device_t));
                if (new_devices == NULL) {
                    pclose(fp);
                    network_device_list_free(list);
                    return -1;
                }
                list->devices = new_devices;
            }
            
            list->devices[list->count] = device;
            list->count++;
        }
    }
    
    pclose(fp);
    list->capacity = capacity;
    return 0;
}

char *network_device_list_to_json(const network_device_list_t *list) {
    if (list == NULL || list->count == 0) {
        char *json = malloc(3);
        if (json) {
            strcpy(json, "[]");
        }
        return json;
    }
    
    // 估算 JSON 大小
    size_t size = 1024 * list->count;
    char *json = malloc(size);
    if (json == NULL) {
        return NULL;
    }
    
    strcpy(json, "[");
    char *p = json + 1;
    
    for (size_t i = 0; i < list->count; i++) {
        const network_device_t *dev = &list->devices[i];
        
        int written = snprintf(p, size - (p - json),
            "%s{"
            "\"device\":\"%s\","
            "\"type\":\"%s\","
            "\"state\":\"%s\","
            "\"connection\":\"%s\","
            "\"ip\":\"%s\","
            "\"mac\":\"%s\","
            "\"speed\":\"%s\","
            "\"ssid\":\"%s\","
            "\"signal\":%d"
            "}",
            (i > 0) ? "," : "",
            dev->device,
            dev->type,
            dev->state,
            dev->connection,
            dev->ip,
            dev->mac,
            dev->speed,
            dev->ssid,
            dev->signal
        );
        
        if (written < 0 || (size_t)written >= size - (p - json)) {
            free(json);
            return NULL;
        }
        
        p += written;
    }
    
    strcpy(p, "]");
    return json;
}
