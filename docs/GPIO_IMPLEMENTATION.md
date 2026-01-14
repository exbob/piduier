# GPIO 控制实现方案（使用 libgpiod）

## libgpiod 简介

[libgpiod](https://libgpiod.readthedocs.io/en/latest/) 是 Linux 系统上用于与 GPIO 交互的现代 C 库，它：

- 使用 **GPIO 字符设备接口**（Linux 4.8+ 引入）
- 替代已弃用的 **GPIO sysfs 接口**（`/sys/class/gpio/`）
- 提供更高效、更可靠的 GPIO 操作方式
- 是 Linux 内核推荐的 GPIO 操作标准

## 为什么选择 libgpiod？

### 优势

1. **现代标准**
   - 使用内核推荐的 GPIO 字符设备接口
   - sysfs 接口已被标记为弃用

2. **性能更好**
   - 基于 `ioctl` 的直接内核交互
   - 比文件系统操作更高效

3. **自动资源管理**
   - 关闭文件描述符时自动释放所有资源
   - 避免资源泄漏

4. **功能丰富**
   - 支持事件轮询（边沿检测）
   - 支持批量操作（同时设置/读取多个 GPIO）
   - 支持开漏/开集输出模式

5. **跨平台兼容**
   - 支持所有使用 GPIO 字符设备的 Linux 系统
   - 不限于树莓派

### 缺点

1. **需要额外依赖**
   - 需要安装 `libgpiod-dev` 开发包
   - 但树莓派 OS 通常已预装或易于安装

2. **API 相对复杂**
   - 比简单的文件读写复杂一些
   - 但提供了更好的抽象和错误处理

## 实现示例

### 基本使用流程

```c
#include <gpiod.h>

// 1. 打开 GPIO chip
struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
if (!chip) {
    // 错误处理
}

// 2. 请求 GPIO 线（例如 GPIO 17）
struct gpiod_line *line = gpiod_chip_get_line(chip, 17);
if (!line) {
    // 错误处理
}

// 3. 配置为输出模式，初始值为 LOW
int ret = gpiod_line_request_output(line, "myapp", 0);
if (ret < 0) {
    // 错误处理
}

// 4. 设置输出值
gpiod_line_set_value(line, 1);  // HIGH
gpiod_line_set_value(line, 0);  // LOW

// 5. 读取值（输出模式下也可以读取）
int value = gpiod_line_get_value(line);

// 6. 释放资源
gpiod_line_release(line);
gpiod_chip_close(chip);
```

### 输入模式示例

```c
// 配置为输入模式
int ret = gpiod_line_request_input(line, "myapp");
if (ret < 0) {
    // 错误处理
}

// 读取输入值
int value = gpiod_line_get_value(line);
```

### 事件轮询示例（可选功能）

```c
// 配置为输入模式，监听上升沿
int ret = gpiod_line_request_rising_edge_events(line, "myapp");
if (ret < 0) {
    // 错误处理
}

// 等待事件（阻塞）
struct timespec timeout = { 5, 0 };  // 5秒超时
int ret = gpiod_line_event_wait(line, &timeout);
if (ret > 0) {
    // 事件发生，读取事件
    struct gpiod_line_event event;
    gpiod_line_event_read(line, &event);
}
```

## 在我们的项目中的应用

### 支持的 GPIO 引脚

根据限制，我们只需要支持以下 GPIO 引脚：
- GPIO 5, 6, 16, 17, 22, 23, 24, 25, 26, 27

### 实现函数设计

```c
// GPIO 控制结构
struct gpio_control {
    struct gpiod_chip *chip;
    struct gpiod_line *lines[10];  // 对应 10 个可配置的 GPIO
    int gpio_numbers[10];          // GPIO 编号映射
};

// 初始化 GPIO 控制
int gpio_init(struct gpio_control *ctrl);

// 设置 GPIO 为输出模式
int gpio_set_output(int gpio_num, int initial_value);

// 设置 GPIO 输出值
int gpio_set_value(int gpio_num, int value);

// 读取 GPIO 值
int gpio_get_value(int gpio_num);

// 设置 GPIO 为输入模式
int gpio_set_input(int gpio_num);

// 清理资源
void gpio_cleanup(struct gpio_control *ctrl);
```

### GPIO 编号映射

树莓派 GPIO 编号到 libgpiod 线号的映射：
- GPIO 5  -> line 5
- GPIO 6  -> line 6
- GPIO 16 -> line 16
- GPIO 17 -> line 17
- GPIO 22 -> line 22
- GPIO 23 -> line 23
- GPIO 24 -> line 24
- GPIO 25 -> line 25
- GPIO 26 -> line 26
- GPIO 27 -> line 27

## 编译配置

### CMakeLists.txt 更新

```cmake
# 查找 libgpiod
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

# 链接库
target_link_libraries(server pthread ${GPIOD_LIBRARIES})
target_include_directories(server PRIVATE ${GPIOD_INCLUDE_DIRS})
```

### 安装依赖

```bash
# 树莓派 OS
sudo apt-get update
sudo apt-get install libgpiod-dev

# 验证安装
pkg-config --modversion libgpiod
```

## 错误处理

libgpiod 使用返回值表示错误：
- 成功：返回 0 或正数
- 失败：返回负数，设置 `errno`

建议的错误处理：
```c
int ret = gpiod_line_request_output(line, "myapp", 0);
if (ret < 0) {
    fprintf(stderr, "Failed to request GPIO line: %s\n", strerror(errno));
    return -1;
}
```

## 性能考虑

- libgpiod 使用 `ioctl` 系统调用，比文件系统操作快
- 批量操作可以一次设置/读取多个 GPIO，提高效率
- 事件轮询使用内核机制，比轮询文件更高效

## 总结

**推荐使用 libgpiod**，因为：
1. 这是 Linux 内核推荐的现代标准
2. 性能更好，功能更丰富
3. 自动资源管理，更安全
4. 虽然需要额外依赖，但易于安装
5. 为未来扩展（如事件轮询）提供了基础
