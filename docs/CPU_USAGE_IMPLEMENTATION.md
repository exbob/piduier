# CPU 占用率计算实现方案 ✅ 已实现

## 需求回顾

- **功能**: 实时显示 CPU 使用百分比
- **更新频率**: 需要定期刷新（建议 1-2 秒）
- **显示方式**: 可以选择显示总体 CPU 占用率或每个核心的占用率

## 实现原理

### `/proc/stat` 文件格式

Linux 系统通过 `/proc/stat` 文件提供 CPU 时间统计信息。文件内容示例：

```
cpu  123456 0 23456 567890 12345 0 1234 0 0 0
cpu0 12345 0 2345 56789 1234 0 123 0 0 0
cpu1 12345 0 2345 56789 1234 0 123 0 0 0
cpu2 12345 0 2345 56789 1234 0 123 0 0 0
cpu3 12345 0 2345 56789 1234 0 123 0 0 0
...
```

每行的字段含义（从左到右）：
1. `cpu` 或 `cpuN`: CPU 标识（`cpu` 表示总体，`cpu0`、`cpu1` 等表示各个核心）
2. `user`: 用户态时间（jiffies）
3. `nice`: 低优先级用户态时间
4. `system`: 内核态时间
5. `idle`: 空闲时间
6. `iowait`: 等待 I/O 的时间
7. `irq`: 硬中断时间
8. `softirq`: 软中断时间
9. `steal`: 虚拟化环境中被其他虚拟机占用的时间
10. `guest`: 运行虚拟客户机的时间
11. `guest_nice`: 低优先级虚拟客户机时间

### 计算方法

CPU 占用率 = (总时间 - 空闲时间) / 总时间 × 100%

其中：
- **总时间** = user + nice + system + idle + iowait + irq + softirq + steal
- **空闲时间** = idle + iowait（通常只计算 idle，iowait 表示等待 I/O，不算真正的空闲）

更精确的计算：
- **总时间** = user + nice + system + idle + iowait + irq + softirq + steal
- **使用时间** = 总时间 - idle
- **CPU 占用率** = 使用时间 / 总时间 × 100%

### 采样方法

由于 `/proc/stat` 显示的是累计时间，需要**两次采样**并计算差值：

1. **第一次采样**：读取当前 CPU 时间
2. **等待一段时间**（建议 1 秒）
3. **第二次采样**：再次读取 CPU 时间
4. **计算差值**：第二次 - 第一次
5. **计算占用率**：(总时间差 - 空闲时间差) / 总时间差 × 100%

## 实现方案

### 方案 A：总体 CPU 占用率（推荐用于监控面板）

只显示整个系统的 CPU 占用率，不区分各个核心。

#### 优点
- 实现简单
- 数据量小，适合前端显示
- 符合大多数监控场景的需求

#### 缺点
- 无法看到各个核心的负载分布
- 多核系统中可能掩盖单核高负载问题

#### 实现代码示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

// 读取总体 CPU 时间
int read_cpu_time(cpu_time_t *cpu) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        return -1;
    }
    
    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    
    // 解析 "cpu" 行
    if (sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &cpu->user, &cpu->nice, &cpu->system, &cpu->idle,
               &cpu->iowait, &cpu->irq, &cpu->softirq, &cpu->steal) != 8) {
        return -1;
    }
    
    return 0;
}

// 计算 CPU 占用率
double calculate_cpu_usage(cpu_time_t *prev, cpu_time_t *curr) {
    unsigned long prev_total = prev->user + prev->nice + prev->system +
                               prev->idle + prev->iowait + prev->irq +
                               prev->softirq + prev->steal;
    unsigned long curr_total = curr->user + curr->nice + curr->system +
                               curr->idle + curr->iowait + curr->irq +
                               curr->softirq + curr->steal;
    
    unsigned long prev_idle = prev->idle + prev->iowait;
    unsigned long curr_idle = curr->idle + curr->iowait;
    
    unsigned long total_delta = curr_total - prev_total;
    unsigned long idle_delta = curr_idle - prev_idle;
    
    if (total_delta == 0) {
        return 0.0;  // 避免除零
    }
    
    double usage = 100.0 * (1.0 - (double)idle_delta / (double)total_delta);
    return usage;
}

// 获取 CPU 占用率（需要调用两次，中间间隔 1 秒）
double get_cpu_usage(void) {
    cpu_time_t prev, curr;
    
    // 第一次采样
    if (read_cpu_time(&prev) != 0) {
        return -1.0;  // 错误
    }
    
    // 等待 1 秒
    sleep(1);
    
    // 第二次采样
    if (read_cpu_time(&curr) != 0) {
        return -1.0;  // 错误
    }
    
    // 计算占用率
    return calculate_cpu_usage(&prev, &curr);
}
```

### 方案 B：多核 CPU 占用率

显示每个核心的 CPU 占用率，适合需要详细监控的场景。

#### 优点
- 可以看到各个核心的负载分布
- 便于发现单核高负载问题
- 适合性能分析和调优

#### 缺点
- 实现相对复杂
- 数据量大，前端显示需要更多空间
- 对于简单监控可能信息过载

#### 实现代码示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CPUS 16

typedef struct {
    int num_cores;
    cpu_time_t cores[MAX_CPUS];
    cpu_time_t total;
} cpu_info_t;

// 读取所有 CPU 核心时间
int read_all_cpu_time(cpu_info_t *info) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        return -1;
    }
    
    char line[256];
    int core_count = 0;
    
    // 读取总体 CPU
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }
    
    if (sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &info->total.user, &info->total.nice, &info->total.system,
               &info->total.idle, &info->total.iowait, &info->total.irq,
               &info->total.softirq, &info->total.steal) != 8) {
        fclose(f);
        return -1;
    }
    
    // 读取各个核心
    while (fgets(line, sizeof(line), f) && core_count < MAX_CPUS) {
        if (strncmp(line, "cpu", 3) != 0) {
            break;  // 不是 CPU 行，结束
        }
        
        unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
        if (sscanf(line, "cpu%d %lu %lu %lu %lu %lu %lu %lu %lu",
                   &core_count, &user, &nice, &system, &idle,
                   &iowait, &irq, &softirq, &steal) == 9) {
            info->cores[core_count].user = user;
            info->cores[core_count].nice = nice;
            info->cores[core_count].system = system;
            info->cores[core_count].idle = idle;
            info->cores[core_count].iowait = iowait;
            info->cores[core_count].irq = irq;
            info->cores[core_count].softirq = softirq;
            info->cores[core_count].steal = steal;
            core_count++;
        }
    }
    
    fclose(f);
    info->num_cores = core_count;
    return 0;
}

// 计算所有核心的占用率
int get_all_cpu_usage(double *usage_array, int max_cores) {
    cpu_info_t prev, curr;
    
    // 第一次采样
    if (read_all_cpu_time(&prev) != 0) {
        return -1;
    }
    
    // 等待 1 秒
    sleep(1);
    
    // 第二次采样
    if (read_all_cpu_time(&curr) != 0) {
        return -1;
    }
    
    // 计算总体占用率
    usage_array[0] = calculate_cpu_usage(&prev.total, &curr.total);
    
    // 计算各个核心占用率
    int num_cores = prev.num_cores < curr.num_cores ? prev.num_cores : curr.num_cores;
    if (num_cores > max_cores - 1) {
        num_cores = max_cores - 1;
    }
    
    for (int i = 0; i < num_cores; i++) {
        usage_array[i + 1] = calculate_cpu_usage(&prev.cores[i], &curr.cores[i]);
    }
    
    return num_cores + 1;  // 返回核心数 + 1（总体）
}
```

## 推荐方案

### 推荐：**方案 A（总体 CPU 占用率）+ 可选多核显示**

#### 理由
1. **符合监控面板需求**
   - 监控面板主要显示总体系统状态
   - 总体占用率足以反映系统负载

2. **实现简单**
   - 代码量少，易于维护
   - 性能开销小

3. **前端友好**
   - 单个数值易于显示
   - 可以配合进度条或仪表盘显示

4. **可扩展**
   - 如果未来需要，可以添加多核显示功能
   - 可以在详细页面显示各核心占用率

### 实现建议

1. **主监控面板**：显示总体 CPU 占用率
2. **详细页面（可选）**：显示各核心占用率
3. **采样间隔**：1 秒（平衡实时性和系统开销）
4. **缓存机制**：后端可以缓存上一次采样结果，避免每次请求都等待 1 秒

### 缓存机制优化

为了避免每次 API 请求都等待 1 秒，可以实现后台定时采样：

```c
// 全局变量
static cpu_time_t cached_prev, cached_curr;
static double cached_usage = 0.0;
static time_t last_update = 0;

// 后台线程定期更新（每 1 秒）
void* cpu_monitor_thread(void *arg) {
    while (1) {
        read_cpu_time(&cached_prev);
        sleep(1);
        read_cpu_time(&cached_curr);
        cached_usage = calculate_cpu_usage(&cached_prev, &cached_curr);
        last_update = time(NULL);
    }
    return NULL;
}

// API 直接返回缓存值
double get_cpu_usage_cached(void) {
    return cached_usage;
}
```

## API 设计

### CPU 占用率 API

```
GET /api/system/cpu
返回: {
  "usage": 15.5,           // 总体 CPU 占用率（%）
  "cores": [                // 可选：各核心占用率
    15.2,
    16.1,
    14.8,
    15.9
  ],
  "timestamp": 1234567890   // 采样时间戳
}
```

或者集成到系统信息 API：

```
GET /api/system/info
返回: {
  ...
  "cpu_usage": 15.5,
  ...
}
```

## 注意事项

1. **首次调用延迟**
   - 第一次调用需要等待 1 秒采样
   - 建议使用缓存机制，后台定期更新

2. **精度问题**
   - 采样间隔越短，精度越高，但系统开销越大
   - 1 秒间隔是较好的平衡点

3. **多核系统**
   - 总体占用率 = 所有核心的平均占用率
   - 如果某个核心 100% 使用，总体占用率 = 100% / 核心数

4. **iowait 处理**
   - 通常只计算 `idle` 作为空闲时间
   - `iowait` 表示等待 I/O，不算真正的空闲，但也不算 CPU 使用
   - 可以根据需求决定是否包含 `iowait`

## 总结

**推荐使用总体 CPU 占用率方案**，因为：
1. ✅ 满足监控面板的基本需求
2. ✅ 实现简单，易于维护
3. ✅ 性能开销小
4. ✅ 前端显示友好
5. ✅ 可扩展（未来可添加多核显示）

**实现要点**：
- 使用 `/proc/stat` 读取 CPU 时间
- 两次采样计算差值（间隔 1 秒）
- 使用缓存机制避免每次请求都等待
- 计算公式：`(总时间差 - 空闲时间差) / 总时间差 × 100%`
