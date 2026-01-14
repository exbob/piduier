#include "wifi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void wifi_network_list_init(wifi_network_list_t *list) {
    if (list == NULL) return;
    list->networks = NULL;
    list->count = 0;
    list->capacity = 0;
}

void wifi_network_list_free(wifi_network_list_t *list) {
    if (list == NULL) return;
    if (list->networks) {
        free(list->networks);
    }
    list->networks = NULL;
    list->count = 0;
    list->capacity = 0;
}

int wifi_scan(wifi_network_list_t *list) {
    if (list == NULL) {
        return -1;
    }
    
    FILE *fp = popen("nmcli device wifi list --rescan yes 2>/dev/null", "r");
    if (fp == NULL) {
        return -1;
    }
    
    char line[512];
    size_t capacity = 50;
    size_t count = 0;
    
    if (list->networks == NULL) {
        list->networks = malloc(capacity * sizeof(wifi_network_t));
        if (list->networks == NULL) {
            pclose(fp);
            return -1;
        }
    } else {
        capacity = list->capacity;
        count = list->count;
    }
    
    int header_found = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (!header_found) {
            if (strstr(line, "IN-USE") && strstr(line, "SSID")) {
                header_found = 1;
            }
            continue;
        }
        
        if (count >= capacity) {
            capacity *= 2;
            wifi_network_t *new_networks = realloc(list->networks, capacity * sizeof(wifi_network_t));
            if (new_networks == NULL) {
                pclose(fp);
                return -1;
            }
            list->networks = new_networks;
        }
        
        wifi_network_t *net = &list->networks[count];
        memset(net, 0, sizeof(wifi_network_t));
        
        char in_use[8] = {0};
        char mode[16] = {0};
        char rate[32] = {0};
        char bars[16] = {0};
        
        if (sscanf(line, "%7s %63s %31s %15s %d %31s %d %15s %31[^\n]",
                   in_use, net->ssid, net->bssid, mode, &net->channel,
                   rate, &net->signal, bars, net->security) >= 5) {
            net->in_use = (in_use[0] == '*');
            if (strlen(mode) > 0) {
                snprintf(net->mode, sizeof(net->mode), "%s", mode);
            }
            count++;
        }
    }
    
    pclose(fp);
    list->count = count;
    list->capacity = capacity;
    return 0;
}

int wifi_connect(const char *ssid, const char *password) {
    if (ssid == NULL || strlen(ssid) == 0 || strlen(ssid) > 32) {
        return -1;
    }
    
    for (const char *p = ssid; *p; p++) {
        if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' ||
            *p == '(' || *p == ')' || *p == '<' || *p == '>') {
            return -1;
        }
    }
    
    char cmd[512];
    if (password != NULL && strlen(password) > 0) {
        for (const char *p = password; *p; p++) {
            if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' ||
                *p == '(' || *p == ')' || *p == '<' || *p == '>') {
                return -1;
            }
        }
        snprintf(cmd, sizeof(cmd),
                "nmcli device wifi connect \"%s\" password \"%s\" 2>/dev/null",
                ssid, password);
    } else {
        snprintf(cmd, sizeof(cmd),
                "nmcli device wifi connect \"%s\" 2>/dev/null",
                ssid);
    }
    
    int result = system(cmd);
    return (result == 0) ? 0 : -1;
}

int wifi_disconnect(const char *device) {
    char cmd[256];
    if (device != NULL && strlen(device) > 0) {
        snprintf(cmd, sizeof(cmd), "nmcli device disconnect %s 2>/dev/null", device);
    } else {
        snprintf(cmd, sizeof(cmd), "nmcli device disconnect wlan0 2>/dev/null");
    }
    
    int result = system(cmd);
    return (result == 0) ? 0 : -1;
}

char *wifi_network_list_to_json(const wifi_network_list_t *list) {
    if (list == NULL || list->count == 0) {
        char *json = malloc(3);
        if (json) {
            strcpy(json, "[]");
        }
        return json;
    }
    
    size_t size = 1024 * list->count;
    char *json = malloc(size);
    if (json == NULL) {
        return NULL;
    }
    
    strcpy(json, "[");
    char *p = json + 1;
    
    for (size_t i = 0; i < list->count; i++) {
        const wifi_network_t *net = &list->networks[i];
        
        int written = snprintf(p, size - (p - json),
                "%s{"
                "\"ssid\":\"%s\","
                "\"bssid\":\"%s\","
                "\"signal\":%d,"
                "\"security\":\"%s\","
                "\"channel\":%d,"
                "\"mode\":\"%s\","
                "\"in_use\":%d"
                "}",
                (i > 0) ? "," : "",
                net->ssid,
                net->bssid,
                net->signal,
                net->security,
                net->channel,
                net->mode,
                net->in_use);
        
        if (written > 0) {
            p += written;
        }
    }
    
    strcpy(p, "]");
    return json;
}
