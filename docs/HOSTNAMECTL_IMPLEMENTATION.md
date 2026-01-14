# 系统信息实现方案（使用 hostnamectl）✅ 已实现

## 为什么使用 hostnamectl？

### 优势

1. **统一工具**
   - 一个命令获取所有基本系统信息
   - 代码更简洁，维护更容易

2. **信息完整**
   - 主机名（静态、瞬态、漂亮名称）
   - 操作系统信息
   - 内核版本
   - 硬件信息
   - 机器ID和启动ID

3. **现代标准**
   - `hostnamectl` 是 systemd 的一部分
   - 树莓派 OS（基于 Debian）使用 systemd，完全支持
   - 是 Linux 系统推荐的系统信息管理工具

4. **功能完整**
   - 可以读取和设置主机名
   - 可以设置漂亮主机名
   - 可以设置图标名称和机箱类型

## hostnamectl 命令示例

### 读取系统信息

```bash
$ hostnamectl status
 Static hostname: raspberrypi
       Icon name: computer
      Machine ID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
         Boot ID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
Operating System: Raspberry Pi OS GNU/Linux
          Kernel: Linux 6.1.0-rpi7-rpi-v8
    Architecture: aarch64
 Hardware Vendor: Raspberry Pi Foundation
  Hardware Model: Raspberry Pi 4 Model B
```

### 设置主机名

```bash
# 设置静态主机名
sudo hostnamectl set-hostname new-hostname

# 设置漂亮主机名（可选）
sudo hostnamectl set-hostname --pretty "My Raspberry Pi"

# 设置图标名称（可选）
sudo hostnamectl set-hostname --icon-name "raspberry-pi"

# 设置机箱类型（可选）
sudo hostnamectl set-hostname --chassis embedded
```

### 获取特定信息

```bash
# 仅获取静态主机名
hostnamectl hostname

# 获取漂亮主机名
hostnamectl --pretty

# 获取图标名称
hostnamectl --icon-name

# 获取机箱类型
hostnamectl --chassis
```

## 实现方式

### 读取系统信息

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int parse_hostnamectl_status(system_info_t *info) {
    FILE *fp = popen("hostnamectl status", "r");
    if (fp == NULL) return -1;
    
    char line[512];
    memset(info, 0, sizeof(system_info_t));
    
    while (fgets(line, sizeof(line), fp)) {
        // 解析 Static hostname
        if (strstr(line, "Static hostname:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%255[^\n]", info->static_hostname);
            }
        }
        // 解析 Pretty hostname
        else if (strstr(line, "Pretty hostname:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%255[^\n]", info->pretty_hostname);
            }
        }
        // 解析 Icon name
        else if (strstr(line, "Icon name:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%63[^\n]", info->icon_name);
            }
        }
        // 解析 Chassis
        else if (strstr(line, "Chassis:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%63[^\n]", info->chassis);
            }
        }
        // 解析 Machine ID
        else if (strstr(line, "Machine ID:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%63[^\n]", info->machine_id);
            }
        }
        // 解析 Boot ID
        else if (strstr(line, "Boot ID:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%63[^\n]", info->boot_id);
            }
        }
        // 解析 Operating System
        else if (strstr(line, "Operating System:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%255[^\n]", info->operating_system);
            }
        }
        // 解析 Kernel
        else if (strstr(line, "Kernel:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%255[^\n]", info->kernel);
            }
        }
        // 解析 Architecture
        else if (strstr(line, "Architecture:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%63[^\n]", info->architecture);
            }
        }
        // 解析 Hardware Vendor
        else if (strstr(line, "Hardware Vendor:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%127[^\n]", info->hardware_vendor);
            }
        }
        // 解析 Hardware Model
        else if (strstr(line, "Hardware Model:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%127[^\n]", info->hardware_model);
            }
        }
    }
    
    pclose(fp);
    return 0;
}
```

### 仅获取主机名（简单方式）

```c
static char *get_hostname(void) {
    FILE *fp = popen("hostnamectl hostname", "r");
    if (fp == NULL) return NULL;
    
    char *hostname = malloc(256);
    if (hostname == NULL) {
        pclose(fp);
        return NULL;
    }
    
    if (fgets(hostname, 256, fp) == NULL) {
        free(hostname);
        pclose(fp);
        return NULL;
    }
    
    // 移除尾随换行符
    size_t len = strlen(hostname);
    if (len > 0 && hostname[len - 1] == '\n') {
        hostname[len - 1] = '\0';
    }
    
    pclose(fp);
    return hostname;
}
```

### 设置主机名

```c
static int set_hostname(const char *hostname) {
    // 验证主机名格式
    if (hostname == NULL || strlen(hostname) == 0 || strlen(hostname) > 253) {
        return -1;
    }
    
    // 检查危险字符
    for (const char *p = hostname; *p; p++) {
        if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' || 
            *p == '(' || *p == ')' || *p == '<' || *p == '>' || 
            *p == '\n' || *p == '\r' || *p == ' ') {
            return -1;
        }
    }
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "hostnamectl set-hostname %s", hostname);
    int result = system(cmd);
    return (result == 0) ? 0 : -1;
}
```

### 设置漂亮主机名

```c
static int set_pretty_hostname(const char *pretty_hostname) {
    // 验证输入
    if (pretty_hostname == NULL || strlen(pretty_hostname) > 255) {
        return -1;
    }
    
    // 检查危险字符（漂亮主机名可以包含空格，但不能包含命令注入字符）
    for (const char *p = pretty_hostname; *p; p++) {
        if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' || 
            *p == '(' || *p == ')' || *p == '<' || *p == '>' || 
            *p == '\n' || *p == '\r') {
            return -1;
        }
    }
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "hostnamectl set-hostname --pretty \"%s\"", pretty_hostname);
    int result = system(cmd);
    return (result == 0) ? 0 : -1;
}
```

## API 设计

### 读取系统信息

```
GET /api/system/hostname
返回: {
  "static_hostname": "raspberrypi",
  "pretty_hostname": "Raspberry Pi",
  "icon_name": "computer",
  "chassis": "embedded",
  "machine_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "boot_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "operating_system": "Raspberry Pi OS GNU/Linux",
  "kernel": "Linux 6.1.0-rpi7-rpi-v8",
  "architecture": "aarch64",
  "hardware_vendor": "Raspberry Pi Foundation",
  "hardware_model": "Raspberry Pi 4 Model B"
}
```

### 设置主机名

```
POST /api/system/hostname
请求体: {
  "hostname": "new-hostname"
}
注意: 需要 root 权限
```

### 设置漂亮主机名

```
POST /api/system/hostname/pretty
请求体: {
  "pretty_hostname": "My Raspberry Pi"
}
注意: 需要 root 权限，可选功能
```

## 安全考虑

1. **输入验证**
   - 主机名格式验证：长度限制（1-253字符）、字符限制
   - 防止命令注入：检查危险字符（`;`, `&`, `|`, `$`, `` ` ``, `(`, `)`, `<`, `>`等）
   - 主机名不能包含空格（但漂亮主机名可以）

2. **权限要求**
   - 设置主机名需要 root 权限
   - 读取操作不需要 root 权限

3. **错误处理**
   - 检查 `popen()` 返回值
   - 检查 `system()` 返回值
   - 提供清晰的错误信息

## 优势总结

使用 `hostnamectl` 统一实现系统信息功能的优势：

1. ✅ **代码简洁**：一个工具完成所有系统信息获取
2. ✅ **信息完整**：一次获取主机名、操作系统、内核、硬件等所有信息
3. ✅ **标准工具**：systemd 标准工具，广泛支持
4. ✅ **易于维护**：统一的实现方式
5. ✅ **功能强大**：支持读取和设置主机名、漂亮主机名等

## 注意事项

1. **systemd 依赖**
   - `hostnamectl` 是 systemd 的一部分
   - 树莓派 OS 使用 systemd，完全支持
   - 如果系统不使用 systemd，需要备选方案（读取 `/etc/hostname`）

2. **权限要求**
   - 设置操作需要 root 权限
   - 读取操作不需要 root 权限

3. **字段可选性**
   - 某些字段可能为空（如漂亮主机名、硬件信息等）
   - 需要处理空值情况

4. **与现有代码的兼容**
   - 当前代码已使用 `hostnamectl hostname` 读取主机名
   - 可以扩展为使用 `hostnamectl status` 获取完整信息
   - 设置主机名已使用 `hostnamectl set-hostname`，保持一致
