#ifndef DATETIME_H
#define DATETIME_H

#include <stddef.h>

// 日期时间信息结构
typedef struct {
	char local_time[64];       // 本地时间 "YYYY-MM-DD HH:MM:SS"
	char utc_time[64];         // UTC 时间 "YYYY-MM-DD HH:MM:SS"
	char timezone[64];         // 时区 "Asia/Shanghai"
	char timezone_offset[16];  // 时区偏移 "+0800"
	int ntp_synchronized;      // NTP 同步状态 (0/1)
	int ntp_service_active;    // NTP 服务状态 (0/1)
} datetime_info_t;

// 获取日期时间信息（使用 timedatectl）
// 返回 0 成功，-1 失败
int datetime_get_info(datetime_info_t *info);

// 设置日期时间
// datetime 格式: "YYYY-MM-DD HH:MM:SS"
// 返回 0 成功，-1 失败
int datetime_set_time(const char *datetime);

// 设置时区
// timezone 格式: "Asia/Shanghai"
// 返回 0 成功，-1 失败
int datetime_set_timezone(const char *timezone);

// 获取可用时区列表
// 返回分配的字符串数组，调用者需要 free()，数组以 NULL 结尾
char **datetime_get_timezones(size_t *count);

// 释放时区列表
void datetime_free_timezones(char **timezones);

// 将日期时间信息转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *datetime_info_to_json(const datetime_info_t *info);

#endif  // DATETIME_H
