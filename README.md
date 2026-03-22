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
- 编译后的可执行文件、Web 前端与 `zlog.conf` 会自动安装到 `./deploy/` 目录
- 默认 **Release**：应用与 Mongoose 仅输出 **ERROR / INFO** 到当前工作目录下的 `./piduier.log`（由 `zlog.conf` 配置）
- **Debug**：含 **DEBUG** 级别，日志输出到 **stdout**（终端）
- 每次构建前会自动清理 build 目录，确保使用正确的编译器；安装前会刷新 `deploy/zlog.conf` 以匹配当前构建类型

**编译产物**：
```
deploy/
├── piduier          # 可执行文件（主程序）
├── zlog.conf        # zlog 配置（与构建类型对应）
└── web/             # Web 前端文件目录
    ├── index.html
    ├── css/
    │   └── style.css
    └── js/
        └── app.js
```

**部署到树莓派**：将 `deploy/` 目录下的内容（含 `zlog.conf`）同步到目标机即可。详细说明见 `docs/BUILD_AND_DEPLOY.md`

## 运行

**注意**：
- 请在 **`deploy/`** 目录下启动（或保证当前工作目录下存在 `./zlog.conf`，通常与 `piduier` 同目录）
- Web 文件（`index.html` 等）位于 `./deploy/web/`（构建脚本会自动安装）
- 网络配置操作需要 root 权限，其他硬件操作使用 `piduier` 组权限

```bash
# 使用 build.sh 编译后：
cd deploy
./piduier

# 如需网络配置或开发测试，可使用 root：
cd deploy && sudo ./piduier
```

服务器默认监听 `http://0.0.0.0:8000`

### 配置文件与日志路径（可选）

- **zlog 配置文件**：默认读取当前工作目录下的 `./zlog.conf`。可通过以下方式覆盖：
  - 命令行：`-c PATH` 或 `--zlog-conf PATH`
  - 环境变量：`PIDUIER_ZLOG_CONF`（在未指定 `-c` 时生效）
- **Release 日志文件**：默认写入当前工作目录下的 `./piduier.log`（由 `zlog.conf` 中 `%E(PIDUIER_LOG_FILE)` 决定）。可通过以下方式覆盖：
  - 命令行：`-l PATH` 或 `--log-file PATH`
  - 环境变量：`PIDUIER_LOG_FILE`（在未指定 `-l` 时生效；程序启动时会写入该环境变量供 zlog 展开）
- **Debug 构建**：日志走终端，`PIDUIER_LOG_FILE` 对输出目标无影响，但仍可自定义 `-c` / `PIDUIER_ZLOG_CONF`。
- 查看帮助：`./piduier --help`

示例：

```bash
./piduier --zlog-conf /etc/piduier/zlog.conf --log-file /var/log/piduier.log
PIDUIER_ZLOG_CONF=/opt/piduier/zlog.conf PIDUIER_LOG_FILE=/tmp/piduier.log ./piduier
```

## 使用

1. 启动服务器后，在浏览器中访问：`http://<树莓派IP>:8000`
2. 通过 Web 界面监控和管理树莓派：
   - **监控面板**: 查看系统信息、CPU/内存使用率、网络状态等
   - **网络设置**: 配置以太网和 Wi-Fi
   - **HAT-40Pin**: 控制 GPIO、PWM、SPI、I2C、UART 等硬件接口