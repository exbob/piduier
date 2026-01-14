# 日期时间和时区实现方案（使用 timedatectl）✅ 已实现

## 为什么使用 timedatectl？

### 优势

1. **统一工具**
   - 一个命令同时获取日期时间、时区、NTP 状态等信息
   - 代码更简洁，维护更容易

2. **功能完整**
   - 可以读取和设置日期时间
   - 可以读取和设置时区
   - 可以查看 NTP 同步状态
   - 可以列出所有可用时区

3. **现代标准**
   - `timedatectl` 是 systemd 的一部分
   - 树莓派 OS（基于 Debian）使用 systemd，完全支持
   - 是 Linux 系统推荐的日期时间管理工具

4. **信息丰富**
   - 同时提供本地时间和 UTC 时间
   - 显示时区偏移量
   - 显示 NTP 同步状态

## timedatectl 命令示例

### 读取日期时间和时区

```bash
$ timedatectl status
               Local time: Tue 2026-01-13 16:59:51 CST
           Universal time: Tue 2026-01-13 08:59:51 UTC
                 RTC time: Tue 2026-01-13 08:59:49
                Time zone: Asia/Shanghai (CST, +0800)
System clock synchronized: yes
              NTP service: active
          RTC in local TZ: no
```

### 设置日期时间

```bash
# 设置日期和时间
sudo timedatectl set-time "2025-01-04 17:30:00"
```

### 设置时区

```bash
# 设置时区
sudo timedatectl set-timezone Asia/Shanghai
```

### 列出可用时区

```bash
# 列出所有时区
timedatectl list-timezones

# 过滤特定区域
timedatectl list-timezones | grep Asia
```

## 实现方式

### 读取日期时间和时区

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 解析 timedatectl status 输出
typedef struct {
    char local_time[64];      // "2026-01-13 16:59:51 CST"
    char utc_time[64];         // "2026-01-13 08:59:51 UTC"
    char timezone[64];         // "Asia/Shanghai"
    char timezone_offset[16];   // "+0800"
    int ntp_synchronized;      // 1 = yes, 0 = no
    int ntp_service_active;    // 1 = active, 0 = inactive
} datetime_info_t;

static int parse_timedatectl_status(datetime_info_t *info) {
    FILE *fp = popen("timedatectl status", "r");
    if (fp == NULL) return -1;
    
    char line[256];
    memset(info, 0, sizeof(datetime_info_t));
    
    while (fgets(line, sizeof(line), fp)) {
        // 解析 Local time
        if (strstr(line, "Local time:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;  // 跳过 ':'
                while (*colon == ' ' || *colon == '\t') colon++;  // 跳过空格
                sscanf(colon, "%63[^\n]", info->local_time);
            }
        }
        // 解析 Universal time
        else if (strstr(line, "Universal time:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%63[^\n]", info->utc_time);
            }
        }
        // 解析 Time zone
        else if (strstr(line, "Time zone:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                // 格式: "Asia/Shanghai (CST, +0800)"
                sscanf(colon, "%63[^(]", info->timezone);
                // 去除末尾空格
                char *p = info->timezone + strlen(info->timezone) - 1;
                while (p > info->timezone && (*p == ' ' || *p == '\t')) *p-- = '\0';
                
                // 提取时区偏移量
                char *offset_start = strstr(colon, "+");
                if (!offset_start) offset_start = strstr(colon, "-");
                if (offset_start) {
                    sscanf(offset_start, "%15[^)]", info->timezone_offset);
                }
            }
        }
        // 解析 NTP 同步状态
        else if (strstr(line, "System clock synchronized:")) {
            if (strstr(line, "yes")) {
                info->ntp_synchronized = 1;
            }
        }
        // 解析 NTP 服务状态
        else if (strstr(line, "NTP service:")) {
            if (strstr(line, "active")) {
                info->ntp_service_active = 1;
            }
        }
    }
    
    pclose(fp);
    return 0;
}
```

### 设置日期时间

```c
static int set_datetime(const char *datetime) {
    // 验证格式: "YYYY-MM-DD HH:MM:SS"
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "timedatectl set-time \"%s\"", datetime);
    int result = system(cmd);
    return (result == 0) ? 0 : -1;
}
```

### 设置时区

```c
static int set_timezone(const char *timezone) {
    // 验证时区格式（简单验证）
    if (strlen(timezone) == 0 || strlen(timezone) > 64) {
        return -1;
    }
    
    // 检查危险字符
    if (strpbrk(timezone, ";|&$`()<>\"'\\")) {
        return -1;
    }
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "timedatectl set-timezone %s", timezone);
    int result = system(cmd);
    return (result == 0) ? 0 : -1;
}
```

### 列出可用时区

```c
static char *list_timezones(void) {
    FILE *fp = popen("timedatectl list-timezones", "r");
    if (fp == NULL) return NULL;
    
    // 分配足够大的缓冲区
    size_t size = 1024 * 1024;  // 1MB
    char *buffer = malloc(size);
    if (buffer == NULL) {
        pclose(fp);
        return NULL;
    }
    
    size_t len = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        size_t line_len = strlen(line);
        if (len + line_len + 1 >= size) {
            // 缓冲区不够，扩展
            size *= 2;
            char *new_buffer = realloc(buffer, size);
            if (new_buffer == NULL) {
                free(buffer);
                pclose(fp);
                return NULL;
            }
            buffer = new_buffer;
        }
        strcpy(buffer + len, line);
        len += line_len;
    }
    buffer[len] = '\0';
    
    pclose(fp);
    return buffer;
}
```

## API 设计

### 读取日期时间和时区

```
GET /api/system/datetime
返回: {
  "local": "2026-01-13 16:59:51 CST",
  "utc": "2026-01-13 08:59:51 UTC",
  "timezone": "Asia/Shanghai",
  "timezone_offset": "+0800",
  "ntp_synchronized": true,
  "ntp_service": "active"
}
```

### 设置日期时间

```
POST /api/system/datetime
请求体: {
  "datetime": "2025-01-04 17:30:00"
}
```

### 设置时区

```
POST /api/system/timezone
请求体: {
  "timezone": "Asia/Shanghai"
}
```

### 列出可用时区

```
GET /api/system/timezones
返回: [
  "Africa/Abidjan",
  "Africa/Accra",
  ...
  "Asia/Shanghai",
  ...
]
```

## 安全考虑

1. **输入验证**
   - 日期时间格式验证：`YYYY-MM-DD HH:MM:SS`
   - 时区名称验证：检查是否在可用时区列表中
   - 防止命令注入：检查危险字符

2. **权限要求**
   - 设置日期时间和时区需要 root 权限
   - 读取操作不需要 root 权限

3. **错误处理**
   - 检查 `popen()` 返回值
   - 检查 `system()` 返回值
   - 提供清晰的错误信息

## 优势总结

使用 `timedatectl` 统一实现日期时间和时区功能的优势：

1. ✅ **代码简洁**：一个工具完成所有功能
2. ✅ **信息完整**：同时获取时间、时区、NTP 状态
3. ✅ **标准工具**：systemd 标准工具，广泛支持
4. ✅ **易于维护**：统一的实现方式
5. ✅ **功能强大**：支持读取、设置、列表等操作

## 注意事项

1. **systemd 依赖**
   - `timedatectl` 是 systemd 的一部分
   - 树莓派 OS 使用 systemd，完全支持
   - 如果系统不使用 systemd，需要备选方案

2. **权限要求**
   - 设置操作需要 root 权限
   - 读取操作不需要 root 权限

3. **时区列表**
   - 时区列表可能很大（数百个时区）
   - 考虑缓存或分页返回
