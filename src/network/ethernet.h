#ifndef ETHERNET_H
#define ETHERNET_H

// 以太网配置信息结构
typedef struct {
	char connection_name[128];
	char device[16];
	char method[16];  // "auto" (DHCP) 或 "manual" (静态IP)
	char ip[64];
	char netmask[64];
	char gateway[64];
	char dns1[64];
	char dns2[64];
} ethernet_config_t;

// 获取以太网配置
// device: 设备名，如 "eth0"
// config: 输出配置信息
// 返回 0 成功，-1 失败
int ethernet_get_config(const char *device, ethernet_config_t *config);

// 设置以太网为 DHCP 模式
// device: 设备名，如 "eth0"
// connection_name: 连接名称，如果为 NULL 则使用默认连接
// 返回 0 成功，-1 失败
int ethernet_set_dhcp(const char *device, const char *connection_name);

// 设置以太网为静态 IP
// device: 设备名，如 "eth0"
// connection_name: 连接名称，如果为 NULL 则使用默认连接
// ip: IP 地址，如 "192.168.1.100"
// netmask: 子网掩码，如 "255.255.255.0" 或前缀长度如 "24"
// gateway: 网关地址，如 "192.168.1.1"
// dns1: 主 DNS 服务器，可为 NULL
// dns2: 备用 DNS 服务器，可为 NULL
// 返回 0 成功，-1 失败
int ethernet_set_static(const char *device, const char *connection_name,
                        const char *ip, const char *netmask,
                        const char *gateway, const char *dns1,
                        const char *dns2);

// 将配置转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *ethernet_config_to_json(const ethernet_config_t *config);

#endif  // ETHERNET_H
