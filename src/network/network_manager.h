#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <stddef.h>

// 网络设备信息结构
typedef struct {
    char device[16];        // 设备名，如 "eth0", "wlan0"
    char type[16];          // 类型，如 "ethernet", "wifi"
    char state[32];        // 状态，如 "connected", "disconnected"
    char connection[64];   // 连接名
    char ip[64];           // IP 地址
    char mac[32];          // MAC 地址
    char speed[16];        // 速度（以太网）
    char ssid[64];         // SSID（Wi-Fi）
    int signal;            // 信号强度（Wi-Fi，dBm）
} network_device_t;

// 网络设备列表结构
typedef struct {
    network_device_t *devices;
    size_t count;
    size_t capacity;
} network_device_list_t;

// 初始化设备列表
void network_device_list_init(network_device_list_t *list);

// 释放设备列表
void network_device_list_free(network_device_list_t *list);

// 获取所有网络设备状态（使用 nmcli）
// 返回 0 成功，-1 失败
int network_get_devices(network_device_list_t *list);

// 将设备列表转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *network_device_list_to_json(const network_device_list_t *list);

#endif // NETWORK_MANAGER_H
