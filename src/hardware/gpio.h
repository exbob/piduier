#ifndef GPIO_H
#define GPIO_H

#include <stddef.h>

// 支持的 GPIO 引脚列表
#define GPIO_SUPPORTED_PINS_COUNT 28
#define GPIO_SUPPORTED_PINS { \
    0, 1, 2, 3, 4, 5, 6, 7, \
    8, 9, 10, 11, 12, 13, 14, 15, \
    16, 17, 18, 19, 20, 21, 22, 23, \
    24, 25, 26, 27 \
}
#define GPIO_PINCTRL_FUNC_MAX_LEN 4

// GPIO 模式
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
    GPIO_MODE_UNKNOWN = -1
} gpio_mode_t;

// GPIO 值
typedef enum {
    GPIO_VALUE_LOW = 0,
    GPIO_VALUE_HIGH = 1,
    GPIO_VALUE_UNKNOWN = -1
} gpio_value_t;

// GPIO 引脚信息
typedef struct {
    int gpio_num;           // GPIO 编号
    gpio_mode_t mode;       // 模式（输入/输出）
    gpio_value_t value;     // 当前值
    int available;          // 是否可用（1=可用，0=不可用）
} gpio_pin_info_t;

// GPIO 状态列表
typedef struct {
    gpio_pin_info_t *pins;
    size_t count;
} gpio_status_t;

typedef enum {
    GPIO_PULL_OFF = 0,
    GPIO_PULL_UP = 1,
    GPIO_PULL_DOWN = 2,
    GPIO_PULL_UNKNOWN = -1
} gpio_pull_t;

typedef struct {
    int gpio_num;
    gpio_pull_t pull;
    int drive;         // -1 unknown, 0=dl, 1=dh
    int available;     // 1 = available
} gpio_pin_attrs_t;

// 检查 GPIO 引脚是否支持
// 返回 1 支持，0 不支持
int gpio_is_supported(int gpio_num);

// 初始化 GPIO 状态列表
void gpio_status_init(gpio_status_t *status);

// 释放 GPIO 状态列表
void gpio_status_free(gpio_status_t *status);

// 获取所有支持的 GPIO 引脚状态
// 返回 0 成功，-1 失败
int gpio_get_all_status(gpio_status_t *status);

// 获取指定 GPIO 引脚状态
// 返回 0 成功，-1 失败
int gpio_get_pin_status(int gpio_num, gpio_pin_info_t *info);

// 设置 GPIO 引脚为输入模式
// 返回 0 成功，-1 失败
int gpio_set_input(int gpio_num);

// 设置 GPIO 引脚为输出模式
// initial_value: 初始输出值（0=LOW, 1=HIGH）
// 返回 0 成功，-1 失败
int gpio_set_output(int gpio_num, int initial_value);

// 设置 GPIO 输出值（仅输出模式）
// value: 0=LOW, 1=HIGH
// 返回 0 成功，-1 失败
int gpio_set_value(int gpio_num, int value);

// 读取 GPIO 值
// 返回 0=LOW, 1=HIGH, -1 失败
int gpio_get_value(int gpio_num);

// 获取 GPIO 附加属性（上下拉、驱动强度）
// 返回 0 成功，-1 失败
int gpio_get_attrs(int gpio_num, gpio_pin_attrs_t *attrs);

// 设置 GPIO 上下拉，pull 可取 "off" / "up" / "down"
// 返回 0 成功，-1 失败
int gpio_set_pull(int gpio_num, const char *pull);

// 设置 GPIO drive（dh/dl）
// 返回 0 成功，-1 失败
int gpio_set_drive(int gpio_num, const char *drive);

// 启动时按配置校验并修正 GPIO0~GPIO27 的 pinctrl 功能
// pinctrl_funcs[i] 对应 gpio i，值为 ip/op/a0~a8/no
// 返回 0 成功，-1 失败
int gpio_apply_pinctrl_config(const char pinctrl_funcs[][GPIO_PINCTRL_FUNC_MAX_LEN], size_t pin_count);

// 将 GPIO 状态转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *gpio_status_to_json(const gpio_status_t *status);

// 将单个 GPIO 引脚信息转换为 JSON 字符串
// 返回分配的字符串，调用者需要 free()
char *gpio_pin_info_to_json(const gpio_pin_info_t *info);

#endif // GPIO_H
