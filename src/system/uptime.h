#ifndef UPTIME_H
#define UPTIME_H

#include <stddef.h>

// 获取系统运行时间（从 /proc/uptime）
// 返回运行时间（秒），失败返回 -1
long uptime_get_seconds(void);

// 格式化运行时间
// seconds: 运行时间（秒）
// buffer: 输出缓冲区
// size: 缓冲区大小
// 返回格式化的字符串，如 "1 day, 2:30:00"
void uptime_format(long seconds, char *buffer, size_t size);

#endif // UPTIME_H
