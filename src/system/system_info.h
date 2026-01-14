#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stddef.h>

// 系统信息结构
typedef struct {
    char static_hostname[256];
    char pretty_hostname[256];
    char icon_name[64];
    char chassis[64];
    char machine_id[64];
    char boot_id[64];
    char operating_system[256];
    char kernel[256];
    char architecture[64];
    char hardware_vendor[128];
    char hardware_model[128];
} system_info_t;

// 获取系统信息（使用 hostnamectl）
// 返回 0 成功，-1 失败
int system_get_info(system_info_t *info);

// 设置静态主机名
// 返回 0 成功，-1 失败
int system_set_hostname(const char *hostname);

// 设置漂亮主机名
// 返回 0 成功，-1 失败
int system_set_pretty_hostname(const char *pretty_hostname);

// 将系统信息转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *system_info_to_json(const system_info_t *info);

#endif // SYSTEM_INFO_H
