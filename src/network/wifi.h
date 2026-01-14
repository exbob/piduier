#ifndef WIFI_H
#define WIFI_H

#include <stddef.h>

// Wi-Fi 网络信息结构
typedef struct {
    char ssid[64];
    char bssid[32];
    int signal;
    char security[32];
    int channel;
    char mode[16];
    int in_use;
} wifi_network_t;

// Wi-Fi 网络列表结构
typedef struct {
    wifi_network_t *networks;
    size_t count;
    size_t capacity;
} wifi_network_list_t;

// 初始化 Wi-Fi 网络列表
void wifi_network_list_init(wifi_network_list_t *list);

// 释放 Wi-Fi 网络列表
void wifi_network_list_free(wifi_network_list_t *list);

// 扫描 Wi-Fi 网络
// 返回 0 成功，-1 失败
int wifi_scan(wifi_network_list_t *list);

// 连接 Wi-Fi 网络
// ssid: 网络 SSID
// password: 密码，可为 NULL（开放网络）
// 返回 0 成功，-1 失败
int wifi_connect(const char *ssid, const char *password);

// 断开 Wi-Fi 连接
// device: 设备名，如 "wlan0"，如果为 NULL 则断开所有 Wi-Fi 设备
// 返回 0 成功，-1 失败
int wifi_disconnect(const char *device);

// 将 Wi-Fi 网络列表转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *wifi_network_list_to_json(const wifi_network_list_t *list);

#endif // WIFI_H
