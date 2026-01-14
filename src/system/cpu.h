#ifndef CPU_H
#define CPU_H

// CPU 时间结构
typedef struct {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
} cpu_time_t;

// 读取 CPU 时间（从 /proc/stat）
// 返回 0 成功，-1 失败
int cpu_read_time(cpu_time_t *cpu);

// 计算 CPU 占用率（需要两次采样）
// prev: 第一次采样
// curr: 第二次采样
// 返回占用率百分比（0-100），失败返回 -1
double cpu_calculate_usage(const cpu_time_t *prev, const cpu_time_t *curr);

// 获取 CPU 占用率（内部会等待 1 秒进行两次采样）
// 返回占用率百分比（0-100），失败返回 -1
double cpu_get_usage(void);

// 后台更新 CPU 占用率缓存（应该在后台线程中调用）
void cpu_update_cache(void);

#endif // CPU_H
