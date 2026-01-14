#include "ethernet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int netmask_to_prefix(const char *netmask) {
    if (netmask == NULL) return 24;
    
    unsigned int a, b, c, d;
    if (sscanf(netmask, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
        int prefix = 0;
        unsigned int mask = (a << 24) | (b << 16) | (c << 8) | d;
        while (mask & 0x80000000) {
            prefix++;
            mask <<= 1;
        }
        return prefix;
    }
    
    int prefix = atoi(netmask);
    if (prefix > 0 && prefix <= 32) {
        return prefix;
    }
    
    return 24;
}

static int find_connection_name(const char *device, char *connection_name, size_t size) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nmcli -t -f NAME,DEVICE connection show | grep ':%s$' | head -1 | cut -d: -f1", device);
    
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }
    
    if (fgets(connection_name, size, fp) == NULL) {
        pclose(fp);
        return -1;
    }
    
    size_t len = strlen(connection_name);
    if (len > 0 && connection_name[len - 1] == '\n') {
        connection_name[len - 1] = '\0';
    }
    
    pclose(fp);
    return 0;
}

int ethernet_get_config(const char *device, ethernet_config_t *config) {
    if (device == NULL || config == NULL) {
        return -1;
    }
    
    memset(config, 0, sizeof(ethernet_config_t));
    snprintf(config->device, sizeof(config->device), "%s", device);
    
    if (find_connection_name(device, config->connection_name, sizeof(config->connection_name)) != 0) {
        snprintf(config->connection_name, sizeof(config->connection_name), "Wired connection 1");
    }
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "nmcli connection show \"%s\" 2>/dev/null", config->connection_name);
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "ipv4.method:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%15s", config->method);
            }
        } else if (strstr(line, "ipv4.addresses:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                char cidr[64];
                if (sscanf(colon, "%63s", cidr) == 1) {
                    char *slash = strchr(cidr, '/');
                    if (slash) {
                        *slash = '\0';
                        snprintf(config->ip, sizeof(config->ip), "%s", cidr);
                        int prefix = atoi(slash + 1);
                        if (prefix > 0 && prefix <= 32) {
                            unsigned int mask = 0xFFFFFFFF << (32 - prefix);
                            snprintf(config->netmask, sizeof(config->netmask), "%u.%u.%u.%u",
                                    (mask >> 24) & 0xFF, (mask >> 16) & 0xFF,
                                    (mask >> 8) & 0xFF, mask & 0xFF);
                        }
                    }
                }
            }
        } else if (strstr(line, "ipv4.gateway:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                sscanf(colon, "%63s", config->gateway);
            }
        } else if (strstr(line, "ipv4.dns:")) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                char dns_list[256];
                if (sscanf(colon, "%255[^\n]", dns_list) == 1) {
                    char *dns1 = strtok(dns_list, " ,");
                    if (dns1) {
                        snprintf(config->dns1, sizeof(config->dns1), "%s", dns1);
                        char *dns2 = strtok(NULL, " ,");
                        if (dns2) {
                            snprintf(config->dns2, sizeof(config->dns2), "%s", dns2);
                        }
                    }
                }
            }
        }
    }
    
    pclose(fp);
    return 0;
}

int ethernet_set_dhcp(const char *device, const char *connection_name) {
    if (device == NULL) {
        return -1;
    }
    
    char conn_name[128];
    if (connection_name != NULL && strlen(connection_name) > 0) {
        snprintf(conn_name, sizeof(conn_name), "%s", connection_name);
    } else {
        if (find_connection_name(device, conn_name, sizeof(conn_name)) != 0) {
            snprintf(conn_name, sizeof(conn_name), "Wired connection 1");
        }
    }
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "nmcli connection modify \"%s\" ipv4.method auto 2>/dev/null", conn_name);
    int result = system(cmd);
    if (result != 0) {
        return -1;
    }
    
    snprintf(cmd, sizeof(cmd), "nmcli connection up \"%s\" 2>/dev/null", conn_name);
    result = system(cmd);
    return (result == 0) ? 0 : -1;
}

int ethernet_set_static(const char *device, const char *connection_name,
                        const char *ip, const char *netmask,
                        const char *gateway, const char *dns1, const char *dns2) {
    if (device == NULL || ip == NULL) {
        return -1;
    }
    
    for (const char *p = ip; *p; p++) {
        if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' ||
            *p == '(' || *p == ')' || *p == '<' || *p == '>') {
            return -1;
        }
    }
    
    char conn_name[128];
    if (connection_name != NULL && strlen(connection_name) > 0) {
        snprintf(conn_name, sizeof(conn_name), "%s", connection_name);
    } else {
        if (find_connection_name(device, conn_name, sizeof(conn_name)) != 0) {
            snprintf(conn_name, sizeof(conn_name), "Wired connection 1");
        }
    }
    
    int prefix = netmask_to_prefix(netmask);
    char cidr[64];
    snprintf(cidr, sizeof(cidr), "%s/%d", ip, prefix);
    
    char dns_str[256] = "";
    if (dns1 != NULL && strlen(dns1) > 0) {
        snprintf(dns_str, sizeof(dns_str), "%s", dns1);
        if (dns2 != NULL && strlen(dns2) > 0) {
            snprintf(dns_str + strlen(dns_str), sizeof(dns_str) - strlen(dns_str), " %s", dns2);
        }
    }
    
    char cmd[1024];
    if (gateway != NULL && strlen(gateway) > 0) {
        if (strlen(dns_str) > 0) {
            snprintf(cmd, sizeof(cmd),
                    "nmcli connection modify \"%s\" ipv4.method manual ipv4.addresses %s ipv4.gateway %s ipv4.dns \"%s\" 2>/dev/null",
                    conn_name, cidr, gateway, dns_str);
        } else {
            snprintf(cmd, sizeof(cmd),
                    "nmcli connection modify \"%s\" ipv4.method manual ipv4.addresses %s ipv4.gateway %s 2>/dev/null",
                    conn_name, cidr, gateway);
        }
    } else {
        if (strlen(dns_str) > 0) {
            snprintf(cmd, sizeof(cmd),
                    "nmcli connection modify \"%s\" ipv4.method manual ipv4.addresses %s ipv4.dns \"%s\" 2>/dev/null",
                    conn_name, cidr, dns_str);
        } else {
            snprintf(cmd, sizeof(cmd),
                    "nmcli connection modify \"%s\" ipv4.method manual ipv4.addresses %s 2>/dev/null",
                    conn_name, cidr);
        }
    }
    
    int result = system(cmd);
    if (result != 0) {
        return -1;
    }
    
    snprintf(cmd, sizeof(cmd), "nmcli connection up \"%s\" 2>/dev/null", conn_name);
    result = system(cmd);
    return (result == 0) ? 0 : -1;
}

char *ethernet_config_to_json(const ethernet_config_t *config) {
    if (config == NULL) {
        return NULL;
    }
    
    size_t size = 1024;
    char *json = malloc(size);
    if (json == NULL) {
        return NULL;
    }
    
    snprintf(json, size,
            "{"
            "\"connection_name\":\"%s\","
            "\"device\":\"%s\","
            "\"method\":\"%s\","
            "\"ip\":\"%s\","
            "\"netmask\":\"%s\","
            "\"gateway\":\"%s\","
            "\"dns1\":\"%s\","
            "\"dns2\":\"%s\""
            "}",
            config->connection_name,
            config->device,
            config->method,
            config->ip,
            config->netmask,
            config->gateway,
            config->dns1,
            config->dns2);
    
    return json;
}
