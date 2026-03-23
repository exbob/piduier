# piduier

`piduier` 是一个面向 RaspberryPi 5 的 40-pin 接口的调试平台。后端基于 C + Mongoose 提供 HTTP 服务，前端通过 Web UI 提供系统监控和接口调试能力，适用于嵌入式软硬件联调场景。

## 1. 概述

### 功能特性

**已实现**

- Web 控制台（Dashboard / 40-pin / About）
- 系统信息展示（主机名、运行时间、系统、架构、时间等）
- 资源监控（CPU、内存）
- 网络状态展示（eth0 / wlan0）
- GPIO 页面与 40-pin 引脚视图
- 后端配置文件加载与校验（JSON）
- 支持通过 `systemd` 作为服务运行

**开发中**

- UART 调试页
- SPI 调试页
- I2C 调试页
- PWM 调试页
- Dashboard 中部分占位指标（如温度、存储）

## 2. 构建

### 2.1 前置依赖

使用交叉编译，在 Debian/Ubuntu 宿主机上可先安装：

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  pkg-config \
  libgpiod-dev \
  network-manager
```

如果要在 x86 主机构建 ARM64 版本，还需安装交叉编译器（命令名需包含 `aarch64-linux-gnu-gcc`）。目标机（树莓派5）上，要使用默认的接口配置，然后通过`raspi-config`命令使能SPI，I2C和UART等接口。

### 2.2 构建命令

项目使用仓库根目录的 `build.sh`：

```bash
# 构建 Release 版本（默认 ARCH=arm64）
./build.sh

# 指定 x86 架构
ARCH=x86 ./build.sh

# 指定 arm64 架构
ARCH=arm64 ./build.sh

# 构建 Debug 版本，带调试信息
./build.sh debug

# 清理
./build.sh clean
```

构建成功后产物位于 `deploy/`：

```text
deploy/
├── piduier          # 可执行文件
├── piduier.conf     # 运行配置
├── install.sh       # 安装脚本（目标机执行）
├── piduier.service  # systemd 服务模板
├── web/             # Web 静态资源
└── uninstall.sh     # 卸载脚本（目标机执行）
```

可以用 `deploy.sh` 脚本把这些文件复制到远程目标机（树莓派5）上，也可以手动复制。调试时，可以直接在目标机上启动 `piduier` ，默认使用当前目录的配置文件 `piduier.conf`。

### 2.3 安装与启动

在目标机用安装脚本部署，用 systemd 启动：

```bash
# 进入目标机执行
sudo ./install.sh

# 可选：自定义安装前缀（默认 /usr/local）
sudo ./install.sh --prefix /usr/local
```

安装脚本会完成：

- 二进制安装到 `<prefix>/bin/piduier`
- 配置安装到 `<prefix>/etc/piduier/piduier.conf`
- 前端资源安装到 `<prefix>/share/piduier/web`
- systemd 服务安装到 `/etc/systemd/system/piduier.service`
- 自动修正配置中的 `web_root` 与 `zlog.log_file` 为安装路径

启动与管理服务：

```bash
sudo systemctl daemon-reload
sudo systemctl start piduier
sudo systemctl status piduier
sudo systemctl enable piduier
```

### 2.4 配置文件说明

程序通过 `-f` 或 `--config` 指定配置文件路径（默认 `./piduier.conf`）：

```bash
./piduier --config /usr/local/etc/piduier/piduier.conf
```

关键字段说明（`piduier.conf`）：

- `http_listen`：监听地址（如 `0.0.0.0`）
- `http_port`：监听端口（如 `8000`）
- `web_root`：前端静态资源目录
- `pinctrl`：GPIO0~GPIO27 的目标功能映射（键为 `gpio0`~`gpio27`，值为 `ip`/`op`/`a0`~`a8`/`no`）
- `zlog.log_file`：日志文件路径（Debug 配置可为空并输出到 stdout）

配置字段缺失或非法时，程序会在启动阶段报错并退出。

`pinctrl` 行为说明：

- 服务启动时会执行 `pinctrl get` 读取 GPIO0~GPIO27 当前功能（每行第二列）。
- 若当前功能与配置不一致，会自动执行 `pinctrl set <gpio> <func>` 修正。
- 包括配置值为 `no` 的情况，也会按配置强制下发为 `no`。

### 2.5 Release 与 Debug 配置区别

项目中对应的模板文件分别是：

- `config/piduier_release.json`
- `config/piduier_debug.json`

主要区别如下：

- 日志输出方式：
  - Release：写入日志文件（`zlog.log_file` 非空，`rules` 使用 `@LOG_FILE@`）。
  - Debug：输出到标准输出（`zlog.log_file` 为空，`rules` 使用 `>stdout`）。
- 使用场景：
  - Release：适合长期运行和 systemd 托管部署。
  - Debug：适合开发联调和前台观察实时日志。
- 其他关键业务配置（如 `http_listen`、`http_port`、`web_root`、`pinctrl`）应保持一致，避免运行行为偏差。

## 3. 使用

### 3.1 访问 Web 控制台

服务启动后，在浏览器打开：

```text
http://<设备IP>:<http_port>
```

其中 `<http_port>` 由配置文件中的 `http_port` 决定（常见为 `8000`）。

### 3.2 主要功能使用

- **Dashboard**
  - 查看设备基础信息、CPU/内存、网络状态
  - 支持页面内手动刷新
  - 提供重启/关机快捷操作按钮
- **40-pin Debug > GPIO**
  - 查看 40-pin 引脚布局与状态
  - 进入 GPIO 页面后执行引脚相关调试操作
- **About**
  - 查看软件版本与说明信息

### 3.3 当前功能状态说明

UART / SPI / I2C / PWM 页面当前为开发中状态，界面已预留但完整调试能力尚未开放。  
使用时请以 Dashboard 与 GPIO 页面能力为主。