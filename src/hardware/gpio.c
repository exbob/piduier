#include "gpio.h"
#include "util/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

// 支持的 GPIO 引脚列表
static const int supported_pins[GPIO_SUPPORTED_PINS_COUNT] = {4, 5, 6, 16, 17, 22, 23, 24, 25, 26, 27};

int gpio_is_supported(int gpio_num) {
    for (int i = 0; i < GPIO_SUPPORTED_PINS_COUNT; i++) {
        if (supported_pins[i] == gpio_num) {
            return 1;
        }
    }
    return 0;
}

void gpio_status_init(gpio_status_t *status) {
    if (status == NULL) return;
    status->pins = NULL;
    status->count = 0;
}

void gpio_status_free(gpio_status_t *status) {
    if (status == NULL) return;
    if (status->pins) {
        free(status->pins);
    }
    status->pins = NULL;
    status->count = 0;
}

static int run_cmd(const char *cmd) {
    if (cmd != NULL && strstr(cmd, "pinctrl set") != NULL) {
        LOG_INFO("[GPIO][pinctrl] executing: %s", cmd);
    }
    int rc = system(cmd);
    if (rc == -1) return -1;
    if (WIFEXITED(rc) && WEXITSTATUS(rc) == 0) return 0;
    return -1;
}

static int run_cmd_output(const char *cmd, char *out, size_t out_size) {
    if (out == NULL || out_size == 0) return -1;
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    size_t n = fread(out, 1, out_size - 1, fp);
    out[n] = '\0';
    int status = pclose(fp);
    if (status == -1) return -1;
    if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0)) return -1;
    return 0;
}

static void to_lowercase(char *s) {
    if (!s) return;
    while (*s) {
        *s = (char) tolower((unsigned char) *s);
        s++;
    }
}

static int pinctrl_get_state(int gpio_num, gpio_mode_t *mode, gpio_value_t *value) {
    char cmd[64];
    char buf[1024];
    snprintf(cmd, sizeof(cmd), "pinctrl get %d 2>/dev/null", gpio_num);
    if (run_cmd_output(cmd, buf, sizeof(buf)) != 0) {
        return -1;
    }

    to_lowercase(buf);
    if (mode) *mode = GPIO_MODE_UNKNOWN;
    if (value) *value = GPIO_VALUE_UNKNOWN;

    if (strstr(buf, " output") || strstr(buf, " = output") ||
        strstr(buf, " op ") || strstr(buf, ": op") || strstr(buf, "op|")) {
        if (mode) *mode = GPIO_MODE_OUTPUT;
    } else if (strstr(buf, " input") || strstr(buf, " = input") ||
               strstr(buf, " ip ") || strstr(buf, ": ip") || strstr(buf, "ip|")) {
        if (mode) *mode = GPIO_MODE_INPUT;
    }

    if (strstr(buf, " hi") || strstr(buf, " hi ") || strstr(buf, " dh") || strstr(buf, "level=1")) {
        if (value) *value = GPIO_VALUE_HIGH;
    } else if (strstr(buf, " lo") || strstr(buf, " lo ") || strstr(buf, " dl") || strstr(buf, "level=0")) {
        if (value) *value = GPIO_VALUE_LOW;
    }

    return 0;
}

static gpio_pull_t parse_pull_from_text(const char *text) {
    if (text == NULL) return GPIO_PULL_UNKNOWN;
    if (strstr(text, " pu") || strstr(text, "|pu") || strstr(text, " pull=up")) {
        return GPIO_PULL_UP;
    }
    if (strstr(text, " pd") || strstr(text, "|pd") || strstr(text, " pull=down")) {
        return GPIO_PULL_DOWN;
    }
    if (strstr(text, " pn") || strstr(text, "|pn") || strstr(text, " pull=none")) {
        return GPIO_PULL_OFF;
    }
    return GPIO_PULL_UNKNOWN;
}

static int parse_drive_from_text(const char *text) {
    if (text == NULL) return -1;
    if (strstr(text, " dh") || strstr(text, "|dh")) return 1;
    if (strstr(text, " dl") || strstr(text, "|dl")) return 0;
    return -1;
}

int gpio_get_all_status(gpio_status_t *status) {
    if (status == NULL) {
        return -1;
    }
    
    gpio_status_init(status);
    
    status->pins = malloc(GPIO_SUPPORTED_PINS_COUNT * sizeof(gpio_pin_info_t));
    if (status->pins == NULL) {
        return -1;
    }
    
    status->count = GPIO_SUPPORTED_PINS_COUNT;
    
    for (int i = 0; i < GPIO_SUPPORTED_PINS_COUNT; i++) {
        gpio_pin_info_t *pin = &status->pins[i];
        pin->gpio_num = supported_pins[i];
        pin->available = 1;
        
        if (gpio_get_pin_status(supported_pins[i], pin) != 0) {
            pin->mode = GPIO_MODE_UNKNOWN;
            pin->value = GPIO_VALUE_UNKNOWN;
        }
    }
    
    return 0;
}

int gpio_get_pin_status(int gpio_num, gpio_pin_info_t *info) {
    if (info == NULL || !gpio_is_supported(gpio_num)) {
        return -1;
    }
    
    info->gpio_num = gpio_num;
    info->available = 1;
    
    return pinctrl_get_state(gpio_num, &info->mode, &info->value);
}

int gpio_set_input(int gpio_num) {
    if (!gpio_is_supported(gpio_num)) {
        return -1;
    }
    
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "pinctrl set %d ip", gpio_num);
    return run_cmd(cmd);
}

int gpio_set_output(int gpio_num, int initial_value) {
    if (!gpio_is_supported(gpio_num)) {
        return -1;
    }
    
    char cmd[64];
    const char *level = (initial_value == 0) ? "dl" : "dh";
    snprintf(cmd, sizeof(cmd), "pinctrl set %d op %s", gpio_num, level);
    return run_cmd(cmd);
}

int gpio_set_value(int gpio_num, int value) {
    if (!gpio_is_supported(gpio_num)) {
        return -1;
    }
    
    char cmd[64];
    const char *level = (value == 0) ? "dl" : "dh";
    snprintf(cmd, sizeof(cmd), "pinctrl set %d op %s", gpio_num, level);
    return run_cmd(cmd);
}

int gpio_get_value(int gpio_num) {
    if (!gpio_is_supported(gpio_num)) {
        return -1;
    }
    
    gpio_mode_t mode = GPIO_MODE_UNKNOWN;
    gpio_value_t val = GPIO_VALUE_UNKNOWN;
    if (pinctrl_get_state(gpio_num, &mode, &val) != 0) {
        return -1;
    }
    if (val == GPIO_VALUE_HIGH) return 1;
    if (val == GPIO_VALUE_LOW) return 0;
    return -1;
}

int gpio_get_attrs(int gpio_num, gpio_pin_attrs_t *attrs) {
    if (attrs == NULL || !gpio_is_supported(gpio_num)) {
        return -1;
    }

    char cmd[64];
    char buf[1024];
    snprintf(cmd, sizeof(cmd), "pinctrl get %d 2>/dev/null", gpio_num);
    if (run_cmd_output(cmd, buf, sizeof(buf)) != 0) {
        attrs->gpio_num = gpio_num;
        attrs->pull = GPIO_PULL_UNKNOWN;
        attrs->drive = -1;
        attrs->available = 0;
        return -1;
    }

    to_lowercase(buf);
    attrs->gpio_num = gpio_num;
    attrs->pull = parse_pull_from_text(buf);
    attrs->drive = parse_drive_from_text(buf);
    attrs->available = 1;
    return 0;
}

int gpio_set_pull(int gpio_num, const char *pull) {
    if (!gpio_is_supported(gpio_num) || pull == NULL) {
        return -1;
    }

    const char *pinctrl_pull = NULL;
    if (strcmp(pull, "up") == 0) {
        pinctrl_pull = "pu";
    } else if (strcmp(pull, "down") == 0) {
        pinctrl_pull = "pd";
    } else if (strcmp(pull, "off") == 0) {
        pinctrl_pull = "pn";
    } else {
        return -1;
    }

    gpio_mode_t mode = GPIO_MODE_UNKNOWN;
    gpio_value_t value = GPIO_VALUE_UNKNOWN;
    if (pinctrl_get_state(gpio_num, &mode, &value) != 0) {
        return -1;
    }

    const char *mode_token = "ip";
    const char *drive_token = NULL;
    if (mode == GPIO_MODE_OUTPUT) {
        mode_token = "op";
        drive_token = (value == GPIO_VALUE_HIGH) ? "dh" : "dl";
    }

    char cmd[96];
    if (drive_token) {
        snprintf(cmd, sizeof(cmd), "pinctrl set %d %s %s %s", gpio_num, mode_token, drive_token, pinctrl_pull);
    } else {
        snprintf(cmd, sizeof(cmd), "pinctrl set %d %s %s", gpio_num, mode_token, pinctrl_pull);
    }
    return run_cmd(cmd);
}

int gpio_set_drive(int gpio_num, const char *drive) {
    if (!gpio_is_supported(gpio_num)) {
        return -1;
    }
    if (drive == NULL || (strcmp(drive, "dh") != 0 && strcmp(drive, "dl") != 0)) {
        return -1;
    }

    gpio_mode_t mode = GPIO_MODE_UNKNOWN;
    gpio_value_t value = GPIO_VALUE_UNKNOWN;
    if (pinctrl_get_state(gpio_num, &mode, &value) != 0) {
        return -1;
    }
    if (mode != GPIO_MODE_OUTPUT) {
        LOG_INFO("[GPIO][pinctrl] skip drive for gpio %d: dh/dl only apply to output (current mode=input or unknown)",
            gpio_num);
        return 0;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "pinctrl set %d op %s", gpio_num, drive);
    return run_cmd(cmd);
}

char *gpio_status_to_json(const gpio_status_t *status) {
    if (status == NULL || status->count == 0) {
        char *json = malloc(3);
        if (json) {
            strcpy(json, "[]");
        }
        return json;
    }
    
    size_t size = 1024 * status->count;
    char *json = malloc(size);
    if (json == NULL) {
        return NULL;
    }
    
    strcpy(json, "[");
    char *p = json + 1;
    
    for (size_t i = 0; i < status->count; i++) {
        const gpio_pin_info_t *pin = &status->pins[i];
        
        const char *mode_str = (pin->mode == GPIO_MODE_INPUT) ? "input" : 
                              (pin->mode == GPIO_MODE_OUTPUT) ? "output" : "unknown";
        const char *value_str = (pin->value == GPIO_VALUE_LOW) ? "0" : 
                               (pin->value == GPIO_VALUE_HIGH) ? "1" : "-1";
        
        gpio_pin_attrs_t attrs;
        int has_attrs = (gpio_get_attrs(pin->gpio_num, &attrs) == 0);
        const char *pull_str = "unknown";
        const char *drive_str = "unknown";
        if (has_attrs) {
            pull_str = (attrs.pull == GPIO_PULL_UP) ? "up" :
                       (attrs.pull == GPIO_PULL_DOWN) ? "down" :
                       (attrs.pull == GPIO_PULL_OFF) ? "off" : "unknown";
            drive_str = (attrs.drive == 1) ? "dh" :
                        (attrs.drive == 0) ? "dl" : "unknown";
        }

        int written = snprintf(p, size - (p - json),
                "%s{"
                "\"gpio_num\":%d,"
                "\"mode\":\"%s\","
                "\"value\":%s,"
                "\"available\":%d,"
                "\"pull\":\"%s\","
                "\"drive\":\"%s\""
                "}",
                (i > 0) ? "," : "",
                pin->gpio_num,
                mode_str,
                value_str,
                pin->available,
                pull_str,
                drive_str);
        
        if (written > 0) {
            p += written;
        }
    }
    
    strcpy(p, "]");
    return json;
}

char *gpio_pin_info_to_json(const gpio_pin_info_t *info) {
    if (info == NULL) {
        return NULL;
    }
    
    const char *mode_str = (info->mode == GPIO_MODE_INPUT) ? "input" : 
                          (info->mode == GPIO_MODE_OUTPUT) ? "output" : "unknown";
    const char *value_str = (info->value == GPIO_VALUE_LOW) ? "0" : 
                           (info->value == GPIO_VALUE_HIGH) ? "1" : "-1";
    
    size_t size = 256;
    char *json = malloc(size);
    if (json == NULL) {
        return NULL;
    }
    
    snprintf(json, size,
            "{"
            "\"gpio_num\":%d,"
            "\"mode\":\"%s\","
            "\"value\":%s,"
            "\"available\":%d"
            "}",
            info->gpio_num,
            mode_str,
            value_str,
            info->available);
    
    return json;
}
