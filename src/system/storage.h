#ifndef STORAGE_H
#define STORAGE_H

// 存储信息结构
typedef struct {
    unsigned long long total;      // 总容量（字节）
    unsigned long long used;       // 已用容量（字节）
    unsigned long long available;  // 可用容量（字节）
    double usage_percent;          // 使用率百分比
    char device[64];               // 设备名称（如 "/dev/mmcblk0p2"）
    char mount_point[256];         // 挂载点（如 "/"）
} storage_info_t;

// 获取存储信息（从 df 命令）
// device: 设备路径，如 "/dev/mmcblk0p2" 或 NULL（使用根分区）
// 返回 0 成功，-1 失败
int storage_get_info(const char *device, storage_info_t *info);

// 将存储信息转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *storage_info_to_json(const storage_info_t *info);

#endif // STORAGE_H
