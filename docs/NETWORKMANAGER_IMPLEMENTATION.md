# 网络配置实现方案（使用 NetworkManager）⏳ 部分实现（状态监控 ✅，配置功能待实现）

## 为什么使用 NetworkManager？

### 优势

1. **统一工具**
   - 一个工具同时管理以太网和 Wi-Fi
   - 代码更简洁，维护更容易

2. **现代标准**
   - NetworkManager 是现代 Linux 系统的标准网络管理工具
   - 树莓派 OS 默认安装并运行 NetworkManager
   - 是 Linux 系统推荐的网络管理方式

3. **功能完整**
   - 支持 DHCP 和静态 IP 配置
   - 支持 Wi-Fi 扫描和连接
   - 支持连接状态监控
   - 自动配置持久化

4. **易于使用**
   - `nmcli` 命令行接口清晰
   - 输出格式易于解析（支持 JSON 输出）
   - 功能丰富，覆盖所有网络管理需求

5. **状态监控**
   - 实时获取连接状态
   - 获取 IP 地址、MAC 地址、速度等信息
   - 获取 Wi-Fi 信号强度、SSID 等

## NetworkManager 命令示例

### 查看设备状态

```bash
# 查看所有设备状态
$ nmcli device status
DEVICE  TYPE      STATE      CONNECTION
eth0    ethernet  connected  Wired connection 1
wlan0   wifi      connected  MyWiFi
lo      loopback  unmanaged  --

# 查看设备详细信息
$ nmcli device show eth0
GENERAL.DEVICE:                         eth0
GENERAL.TYPE:                           ethernet
GENERAL.HWADDR:                         B8:27:EB:XX:XX:XX
GENERAL.MTU:                            1500
GENERAL.STATE:                          100 (connected)
GENERAL.CONNECTION:                     Wired connection 1
IP4.ADDRESS[1]:                         192.168.1.100/24
IP4.GATEWAY:                            192.168.1.1
IP4.DNS[1]:                             8.8.8.8
IP4.DNS[2]:                             8.8.4.4
IP6.ADDRESS[1]:                         fe80::ba27:ebff:fexx:xxxx/64
```

### 查看连接配置

```bash
# 查看所有连接
$ nmcli connection show
NAME                UUID                                  TYPE      DEVICE
Wired connection 1  xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx  ethernet  eth0
MyWiFi              xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx  wifi      wlan0

# 查看特定连接的配置
$ nmcli connection show "Wired connection 1"
connection.id:                          Wired connection 1
connection.uuid:                        xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
connection.type:                        ethernet
ipv4.method:                            manual
ipv4.addresses:                         192.168.1.100/24
ipv4.gateway:                           192.168.1.1
ipv4.dns:                               8.8.8.8,8.8.4.4
```

### 配置以太网

```bash
# 设置静态 IP（修改现有连接）
$ sudo nmcli connection modify "Wired connection 1" \
    ipv4.method manual \
    ipv4.addresses 192.168.1.100/24 \
    ipv4.gateway 192.168.1.1 \
    ipv4.dns "8.8.8.8 8.8.4.4"

# 设置 DHCP
$ sudo nmcli connection modify "Wired connection 1" ipv4.method auto

# 应用配置
$ sudo nmcli connection up "Wired connection 1"

# 创建新连接（静态 IP）
$ sudo nmcli connection add \
    type ethernet \
    con-name "Static Ethernet" \
    ifname eth0 \
    ipv4.method manual \
    ipv4.addresses 192.168.1.100/24 \
    ipv4.gateway 192.168.1.1 \
    ipv4.dns "8.8.8.8 8.8.4.4"
```

### Wi-Fi 操作

```bash
# 扫描 Wi-Fi 网络
$ nmcli device wifi list
IN-USE  SSID      MODE   CHAN  RATE       SIGNAL  BARS  SECURITY
        MyWiFi    Infra  6     54 Mbit/s  75      ▂▄▆█  WPA2
        OtherWiFi Infra  11    54 Mbit/s  50      ▂▄▆_  WPA2

# 连接 Wi-Fi
$ sudo nmcli device wifi connect "MyWiFi" password "mypassword"

# 断开 Wi-Fi
$ sudo nmcli device disconnect wlan0

# 查看已保存的 Wi-Fi 连接
$ nmcli connection show
```

### JSON 输出（便于解析）

```bash
# 使用 --json 参数获取 JSON 格式输出
$ nmcli device status --json
[
  {
    "DEVICE": "eth0",
    "TYPE": "ethernet",
    "STATE": "connected",
    "CONNECTION": "Wired connection 1"
  },
  {
    "DEVICE": "wlan0",
    "TYPE": "wifi",
    "STATE": "connected",
    "CONNECTION": "MyWiFi"
  }
]
```

## 实现方式

### 读取网络设备状态

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>  // 或手动解析 JSON

// 网络设备信息结构
typedef struct {
    char device[16];
    char type[16];
    char state[32];
    char connection[128];
    char ip[64];
    char mac[32];
    char speed[16];
    // Wi-Fi 特有
    char ssid[64];
    int signal;
} network_device_t;

// 使用 JSON 输出解析（推荐）
static int get_network_devices_json(network_device_t *devices, int max_devices) {
    FILE *fp = popen("nmcli device status --json", "r");
    if (fp == NULL) return -1;
    
    // 读取 JSON 数据
    char json_buffer[4096];
    size_t len = fread(json_buffer, 1, sizeof(json_buffer) - 1, fp);
    json_buffer[len] = '\0';
    pclose(fp);
    
    // 解析 JSON（使用 json-c 库或手动解析）
    // ... 解析逻辑 ...
    
    return device_count;
}

// 使用文本输出解析（备选）
static int get_network_devices_text(network_device_t *devices, int max_devices) {
    FILE *fp = popen("nmcli device status", "r");
    if (fp == NULL) return -1;
    
    char line[256];
    int count = 0;
    
    // 跳过标题行
    if (fgets(line, sizeof(line), fp) == NULL) {
        pclose(fp);
        return 0;
    }
    
    while (fgets(line, sizeof(line), fp) && count < max_devices) {
        // 解析格式: "eth0    ethernet  connected  Wired connection 1"
        sscanf(line, "%15s %15s %31s %127[^\n]",
               devices[count].device,
               devices[count].type,
               devices[count].state,
               devices[count].connection);
        
        // 获取详细信息
        get_device_details(devices[count].device, &devices[count]);
        
        count++;
    }
    
    pclose(fp);
    return count;
}
```

### 获取设备详细信息

```c
static int get_device_details(const char *device, network_device_t *info) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nmcli device show %s", device);
    
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        // 解析 IP 地址
        if (strstr(line, "IP4.ADDRESS")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                // 格式: "192.168.1.100/24"
                sscanf(colon, "%63[^/]", info->ip);
            }
        }
        // 解析 MAC 地址
        else if (strstr(line, "GENERAL.HWADDR")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%31[^\n]", info->mac);
            }
        }
        // 解析速度（以太网）
        else if (strstr(line, "SPEED")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%15[^\n]", info->speed);
            }
        }
    }
    
    pclose(fp);
    return 0;
}
```

### 配置以太网（静态 IP）

```c
static int set_ethernet_static(const char *interface, const char *ip,
                               const char *netmask, const char *gateway,
                               const char **dns, int dns_count) {
    // 构建 CIDR 格式（如 192.168.1.100/24）
    char cidr[64];
    int prefix = netmask_to_prefix(netmask);  // 转换子网掩码为前缀长度
    snprintf(cidr, sizeof(cidr), "%s/%d", ip, prefix);
    
    // 构建 DNS 字符串
    char dns_str[256] = "";
    for (int i = 0; i < dns_count && i < 3; i++) {
        if (i > 0) strcat(dns_str, " ");
        strcat(dns_str, dns[i]);
    }
    
    // 查找现有连接或创建新连接
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "nmcli connection modify \"Wired connection 1\" "
             "ipv4.method manual "
             "ipv4.addresses %s "
             "ipv4.gateway %s "
             "ipv4.dns \"%s\"",
             cidr, gateway, dns_str);
    
    int result = system(cmd);
    if (result != 0) return -1;
    
    // 应用配置
    snprintf(cmd, sizeof(cmd),
             "nmcli connection up \"Wired connection 1\"");
    result = system(cmd);
    return (result == 0) ? 0 : -1;
}
```

### 配置以太网（DHCP）

```c
static int set_ethernet_dhcp(const char *interface) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "nmcli connection modify \"Wired connection 1\" ipv4.method auto");
    
    int result = system(cmd);
    if (result != 0) return -1;
    
    snprintf(cmd, sizeof(cmd),
             "nmcli connection up \"Wired connection 1\"");
    result = system(cmd);
    return (result == 0) ? 0 : -1;
}
```

### 扫描 Wi-Fi

```c
typedef struct {
    char ssid[64];
    char bssid[32];
    int signal;
    char security[16];
    int channel;
} wifi_network_t;

static int scan_wifi(wifi_network_t *networks, int max_networks) {
    FILE *fp = popen("nmcli device wifi list", "r");
    if (fp == NULL) return -1;
    
    char line[512];
    int count = 0;
    
    // 跳过标题行
    if (fgets(line, sizeof(line), fp) == NULL) {
        pclose(fp);
        return 0;
    }
    
    while (fgets(line, sizeof(line), fp) && count < max_networks) {
        // 解析格式: "        MyWiFi    Infra  6     54 Mbit/s  75      ▂▄▆█  WPA2"
        char in_use[8], mode[16], rate[32], bars[16];
        sscanf(line, "%7s %63s %15s %d %31s %d %15s %15[^\n]",
               in_use, networks[count].ssid, mode, &networks[count].channel,
               rate, &networks[count].signal, bars, networks[count].security);
        
        count++;
    }
    
    pclose(fp);
    return count;
}
```

### 连接 Wi-Fi

```c
static int connect_wifi(const char *ssid, const char *password) {
    // 验证输入
    if (ssid == NULL || strlen(ssid) == 0 || strlen(ssid) > 32) {
        return -1;
    }
    
    // 检查危险字符
    if (strpbrk(ssid, ";|&$`()<>\"'\\")) {
        return -1;
    }
    
    char cmd[512];
    if (password != NULL && strlen(password) > 0) {
        // 验证密码
        if (strpbrk(password, ";|&$`()<>\"'\\")) {
            return -1;
        }
        snprintf(cmd, sizeof(cmd),
                 "nmcli device wifi connect \"%s\" password \"%s\"",
                 ssid, password);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "nmcli device wifi connect \"%s\"",
                 ssid);
    }
    
    int result = system(cmd);
    return (result == 0) ? 0 : -1;
}
```

### 断开 Wi-Fi

```c
static int disconnect_wifi(const char *interface) {
    char cmd[256];
    if (interface != NULL) {
        snprintf(cmd, sizeof(cmd), "nmcli device disconnect %s", interface);
    } else {
        snprintf(cmd, sizeof(cmd), "nmcli device disconnect wlan0");
    }
    
    int result = system(cmd);
    return (result == 0) ? 0 : -1;
}
```

## API 设计

### 获取网络设备状态

```
GET /api/network/devices
返回: [
  {
    "device": "eth0",
    "type": "ethernet",
    "state": "connected",
    "connection": "Wired connection 1",
    "ip": "192.168.1.100",
    "mac": "b8:27:eb:xx:xx:xx",
    "speed": "1000"
  },
  {
    "device": "wlan0",
    "type": "wifi",
    "state": "connected",
    "connection": "MyWiFi",
    "ip": "192.168.1.101",
    "mac": "dc:a6:32:xx:xx:xx",
    "ssid": "MyWiFi",
    "signal": -45
  }
]
```

### 配置以太网

```
POST /api/network/ethernet/config
请求体: {
  "interface": "eth0",
  "mode": "static",
  "ip": "192.168.1.100",
  "netmask": "255.255.255.0",
  "gateway": "192.168.1.1",
  "dns": ["8.8.8.8", "8.8.4.4"]
}
或
{
  "interface": "eth0",
  "mode": "dhcp"
}
```

### Wi-Fi 操作

```
GET /api/network/wifi/scan
返回: [
  {
    "ssid": "MyWiFi",
    "bssid": "aa:bb:cc:dd:ee:ff",
    "signal": -45,
    "security": "WPA2",
    "channel": 6
  }
]

POST /api/network/wifi/connect
请求体: {
  "ssid": "MyWiFi",
  "password": "mypassword"
}

POST /api/network/wifi/disconnect
请求体: {
  "interface": "wlan0"  // 可选
}
```

## 安全考虑

1. **输入验证**
   - IP 地址格式验证
   - SSID 和密码长度限制
   - 防止命令注入：检查危险字符

2. **权限要求**
   - 所有配置操作需要 root 权限
   - 读取操作不需要 root 权限

3. **错误处理**
   - 检查 `popen()` 和 `system()` 返回值
   - 提供清晰的错误信息

## 优势总结

使用 NetworkManager 统一实现网络功能的优势：

1. ✅ **统一工具**：一个工具管理以太网和 Wi-Fi
2. ✅ **现代标准**：Linux 系统推荐的网络管理工具
3. ✅ **功能完整**：支持所有网络管理需求
4. ✅ **易于使用**：命令行接口清晰，支持 JSON 输出
5. ✅ **配置持久化**：自动保存配置
6. ✅ **状态监控**：实时获取连接状态和信息

## 注意事项

1. **NetworkManager 服务**
   - 需要 NetworkManager 服务运行
   - 树莓派 OS 通常默认安装并运行
   - 可以通过 `systemctl status NetworkManager` 检查

2. **权限要求**
   - 配置操作需要 root 权限
   - 读取操作不需要 root 权限

3. **连接名称**
   - NetworkManager 使用连接名称（connection name）管理配置
   - 需要处理连接名称的查找和创建

4. **JSON 输出**
   - `nmcli` 支持 `--json` 参数，便于解析
   - 可以考虑使用 `json-c` 库解析 JSON 输出

5. **备选方案**
   - 如果系统没有 NetworkManager，可以回退到直接修改配置文件
   - 但推荐使用 NetworkManager，因为它是现代标准
