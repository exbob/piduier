#ifndef MEMORY_H
#define MEMORY_H

// 内存信息结构
typedef struct {
    unsigned long total;        // 总内存（KB）
    unsigned long used;         // 已用内存（KB）
    unsigned long free;         // 空闲内存（KB）
    unsigned long available;    // 可用内存（KB）
    double usage_percent;       // 使用率百分比
} memory_info_t;

// 获取内存信息（从 /proc/meminfo）
// 返回 0 成功，-1 失败
int memory_get_info(memory_info_t *info);

// 将内存信息转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *memory_info_to_json(const memory_info_t *info);

#endif // MEMORY_H
