# 树莓派监控软件 (piduier)

基于 Mongoose 的树莓派监控软件，提供通过 Web 界面监控和管理树莓派的功能。

## 系统环境配置

### 1. 系统依赖安装

安装必要的系统软件包：

```bash
sudo apt-get update
sudo apt-get install -y \
    libgpiod-dev \
    network-manager \
    build-essential \
    cmake \
    pkg-config
```

### 2. 硬件接口启用

使用 `raspi-config` 启用硬件接口：

```bash
sudo raspi-config
```

在界面中启用：
- **SPI**: `Interface Options` → `SPI` → `Enable`
- **I2C**: `Interface Options` → `I2C` → `Enable`
- **Serial Port**: `Interface Options` → `Serial Port` → `Enable`（用于 UART）

或者使用命令行：

```bash
# 启用 SPI
sudo raspi-config nonint do_spi 0

# 启用 I2C
sudo raspi-config nonint do_i2c 0

# 启用串口（UART）
sudo raspi-config nonint do_serial 0
```

### 3. 权限配置

统一使用 `piduier` 用户组访问所有硬件接口：

```bash
# 1. 创建 piduier 组（如果不存在）
sudo groupadd piduier

# 2. 将用户添加到 piduier 组
sudo usermod -aG piduier $USER

# 3. 配置 udev 规则（统一使用 piduier 组）
sudo tee /etc/udev/rules.d/99-piduier.rules > /dev/null <<EOF
# GPIO
SUBSYSTEM=="gpio", GROUP="piduier", MODE="0664"
KERNEL=="gpiochip*", GROUP="piduier", MODE="0664"

# PWM
SUBSYSTEM=="pwm", GROUP="piduier", MODE="0664"
KERNEL=="pwmchip*", GROUP="piduier", MODE="0664"

# SPI
KERNEL=="spidev*", GROUP="piduier", MODE="0664"

# I2C
KERNEL=="i2c-[0-9]*", GROUP="piduier", MODE="0664"

# UART
KERNEL=="ttyAMA*", GROUP="piduier", MODE="0664"
EOF

# 4. 重新加载 udev 规则
sudo udevadm control --reload-rules
sudo udevadm trigger

# 5. 重新登录以使组权限生效
# 或者使用: newgrp piduier
```

**验证权限**（重新登录后）：

```bash
# 检查用户组
groups

# 检查设备权限
ls -l /dev/gpiochip0
ls -l /dev/spidev0.0
ls -l /dev/i2c-1
ls -l /dev/ttyAMA0
```

### 4. NetworkManager 服务

确保 NetworkManager 服务正在运行：

```bash
sudo systemctl status NetworkManager
sudo systemctl enable NetworkManager
```

## 编译

支持交叉编译，通过 `ARCH` 环境变量指定架构：

**x86 架构（默认）：**
```bash
./build.sh
# 或
ARCH=x86 ./build.sh
```

**ARM64 架构：**
```bash
ARCH=arm64 ./build.sh
```

**Debug 构建（日志 DEBUG 级别输出到终端）：**
```bash
./build.sh debug
```

**清理构建文件：**
```bash
./build.sh clean
```

**说明：**
- CMake 生成的所有文件都放在 `./build` 目录下（中间文件，不需要部署）
- 编译后的可执行文件、Web 前端与 **`piduier.conf`（JSON 应用配置）** 会自动安装到 `./deploy/` 目录
- 默认 **Release**：监听地址与端口、zlog 规则等均由 `piduier.conf` 指定；应用与 Mongoose 通常输出 **ERROR / INFO** 到 `zlog.log_file` 配置的路径（模板默认为当前目录下 `./piduier.log`）
- **Debug**：含 **DEBUG** 级别，日志输出到 **stdout**（终端），仍由 `piduier.conf` 中的 `zlog` 段描述
- 每次构建前会自动清理 build 目录，确保使用正确的编译器；安装前会刷新 `deploy/piduier.conf` 以匹配当前构建类型

**编译产物**：
```
deploy/
├── piduier          # 可执行文件（主程序）
├── piduier.conf     # JSON 配置（http_listen / http_port / zlog 等，与构建类型对应）
└── web/             # Web 前端文件目录
    ├── index.html
    ├── css/
    │   └── style.css
    └── js/
        └── app.js
```

**部署到树莓派**：将 `deploy/` 目录下的内容（含 **`piduier.conf`**）同步到目标机即可。详细说明见 `docs/BUILD_AND_DEPLOY.md`

## 运行

**注意**：
- 请在 **`deploy/`** 目录下启动（或保证当前工作目录下存在 **`./piduier.conf`**，通常与 `piduier` 同目录）
- Web 文件（`index.html` 等）位于 `./deploy/web/`（构建脚本会自动安装）
- 网络配置操作需要 root 权限，其他硬件操作使用 `piduier` 组权限

```bash
# 使用 build.sh 编译后：
cd deploy
./piduier

# 如需网络配置或开发测试，可使用 root：
cd deploy && sudo ./piduier
```

HTTP 监听地址与端口由 **`piduier.conf`** 中的 **`http_listen`**、**`http_port`** 指定（安装模板默认为 `0.0.0.0` / `8000`）。

### 配置文件（`piduier.conf`）

- **格式**：JSON。根级必填字段包括 **`http_listen`**（字符串）、**`http_port`**（整数 1–65535）、**`zlog`**（对象）。
- **`zlog`**：包含结构化 zlog 配置（`global`、`formats`、`rules` 对象/数组）及 **`log_file`**（字符串）。文件类规则中使用占位符 **`@LOG_FILE@`**，启动时替换为 `log_file` 的实际路径；不使用环境变量。
- **缺失或非法字段**：程序启动前校验失败并打印错误，**无静默默认值**。
- **命令行**：仅用于指定配置文件路径——**`-f PATH`** 或 **`--config PATH`**（默认 `./piduier.conf`）。不使用环境变量覆盖配置。
- 查看帮助：`./piduier --help`

示例：

```bash
cd deploy
./piduier
./piduier --config /etc/piduier/piduier.conf
```

仓库内模板：`config/piduier_release.json`、`config/piduier_debug.json`（安装时复制为 `deploy/piduier.conf`）。

## 使用

1. 启动服务器后，在浏览器中访问：`http://<树莓派IP>:<http_port>`（默认端口见 `piduier.conf`）
2. 通过 Web 界面监控和管理树莓派：
   - **监控面板**: 查看系统信息、CPU/内存使用率、网络状态等
   - **网络设置**: 配置以太网和 Wi-Fi
   - **HAT-40Pin**: 控制 GPIO、PWM、SPI、I2C、UART 等硬件接口