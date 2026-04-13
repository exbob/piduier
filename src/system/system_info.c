#include "system_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void parse_hostnamectl_line(const char *line, system_info_t *info)
{
	char *colon;

	if ((colon = strstr(line, "Static hostname:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%255[^\n]", info->static_hostname);
		}
	} else if ((colon = strstr(line, "Icon name:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%63[^\n]", info->icon_name);
		}
	} else if ((colon = strstr(line, "Machine ID:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%63[^\n]", info->machine_id);
		}
	} else if ((colon = strstr(line, "Boot ID:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%63[^\n]", info->boot_id);
		}
	} else if ((colon = strstr(line, "Operating System:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%255[^\n]", info->operating_system);
		}
	} else if ((colon = strstr(line, "Kernel:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%255[^\n]", info->kernel);
		}
	} else if ((colon = strstr(line, "Architecture:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%63[^\n]", info->architecture);
		}
	} else if ((colon = strstr(line, "Hardware Vendor:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%127[^\n]", info->hardware_vendor);
		}
	} else if ((colon = strstr(line, "Hardware Model:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%127[^\n]", info->hardware_model);
		}
	} else if ((colon = strstr(line, "Chassis:")) != NULL) {
		colon = strchr(colon, ':');
		if (colon) {
			colon++;
			while (*colon == ' ' || *colon == '\t')
				colon++;
			sscanf(colon, "%63[^\n]", info->chassis);
		}
	}
}

// 获取漂亮主机名（单独命令）
static int get_pretty_hostname(char *buf, size_t size)
{
	FILE *fp = popen("hostnamectl --pretty 2>/dev/null", "r");
	if (fp == NULL) {
		return -1;
	}

	if (fgets(buf, size, fp) == NULL) {
		pclose(fp);
		return -1;
	}

	// 移除换行符
	size_t len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\n') {
		buf[len - 1] = '\0';
	}

	pclose(fp);
	return 0;
}

int system_get_info(system_info_t *info)
{
	if (info == NULL) {
		return -1;
	}

	// 初始化结构
	memset(info, 0, sizeof(system_info_t));

	// 执行 hostnamectl status 命令
	FILE *fp = popen("hostnamectl status 2>/dev/null", "r");
	if (fp == NULL) {
		return -1;
	}

	char line[512];
	while (fgets(line, sizeof(line), fp) != NULL) {
		parse_hostnamectl_line(line, info);
	}

	pclose(fp);

	get_pretty_hostname(info->pretty_hostname, sizeof(info->pretty_hostname));

	if (strlen(info->hardware_vendor) == 0 ||
	    strlen(info->hardware_model) == 0) {
		FILE *f = fopen("/proc/device-tree/model", "r");
		if (f != NULL) {
			char model[256];
			if (fgets(model, sizeof(model), f) != NULL) {
				size_t len = strlen(model);
				if (len > 0 && model[len - 1] == '\n') {
					model[len - 1] = '\0';
				}
				if (strlen(info->hardware_model) == 0) {
					snprintf(info->hardware_model, sizeof(info->hardware_model),
					         "%s", model);
				}
				if (strlen(info->hardware_vendor) == 0) {
					if (strstr(model, "Raspberry Pi") != NULL) {
						snprintf(info->hardware_vendor,
						         sizeof(info->hardware_vendor),
						         "Raspberry Pi Foundation");
					}
				}
			}
			fclose(f);
		}
	}

	return 0;
}

int system_set_hostname(const char *hostname)
{
	if (hostname == NULL || strlen(hostname) == 0 || strlen(hostname) > 253) {
		return -1;
	}

	for (const char *p = hostname; *p; p++) {
		if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' ||
		    *p == '(' || *p == ')' || *p == '<' || *p == '>' || *p == '\n' ||
		    *p == '\r' || *p == ' ') {
			return -1;
		}
	}

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "hostnamectl set-hostname %s 2>/dev/null",
	         hostname);
	int result = system(cmd);
	return (result == 0) ? 0 : -1;
}

int system_set_pretty_hostname(const char *pretty_hostname)
{
	if (pretty_hostname == NULL || strlen(pretty_hostname) > 255) {
		return -1;
	}

	for (const char *p = pretty_hostname; *p; p++) {
		if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' ||
		    *p == '(' || *p == ')' || *p == '<' || *p == '>') {
			return -1;
		}
	}

	char cmd[512];
	snprintf(cmd, sizeof(cmd),
	         "hostnamectl set-hostname --pretty \"%s\" 2>/dev/null",
	         pretty_hostname);
	int result = system(cmd);
	return (result == 0) ? 0 : -1;
}

char *system_info_to_json(const system_info_t *info)
{
	if (info == NULL) {
		return NULL;
	}

	size_t size = 2048;
	char *json = malloc(size);
	if (json == NULL) {
		return NULL;
	}

	snprintf(json, size,
	         "{"
	         "\"static_hostname\":\"%s\","
	         "\"pretty_hostname\":\"%s\","
	         "\"icon_name\":\"%s\","
	         "\"chassis\":\"%s\","
	         "\"machine_id\":\"%s\","
	         "\"boot_id\":\"%s\","
	         "\"operating_system\":\"%s\","
	         "\"kernel\":\"%s\","
	         "\"architecture\":\"%s\","
	         "\"hardware_vendor\":\"%s\","
	         "\"hardware_model\":\"%s\""
	         "}",
	         info->static_hostname, info->pretty_hostname, info->icon_name,
	         info->chassis, info->machine_id, info->boot_id,
	         info->operating_system, info->kernel, info->architecture,
	         info->hardware_vendor, info->hardware_model);

	return json;
}
