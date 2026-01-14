# 树莓派监控软件实现方案 (piduier)

## 项目概述

基于 Mongoose HTTP 服务器，开发一个树莓派监控和管理 Web 应用（piduier），提供实时监控、网络配置和 GPIO 控制功能。

## 功能模块

### 1. 系统监控模块

#### 1.1 显示信息 ✅ 已实现
- ✅ **基本系统信息**: 主机名、操作系统、内核版本、架构、硬件信息等
- ✅ **日期时间**: 当前系统日期和时间
- ✅ **时区**: 当前系统时区设置
- ✅ **CPU 占用率**: 实时 CPU 使用百分比（带后台缓存）
- ✅ **内存占用率**: 实时内存使用百分比（已用/总计）
- ✅ **启动时长**: 系统运行时间（uptime）
- ✅ **CPU 温度**: CPU 温度监控
- ✅ **存储占用率**: 根文件系统存储使用情况
- ✅ **以太网状态**: 连接状态、IP 地址、MAC 地址、速度（从 sysfs 读取）
- ✅ **Wi-Fi 状态**: 连接状态、SSID、IP 地址、信号强度

#### 1.2 实现方式
- **基本系统信息**: 统一使用 `hostnamectl` 命令
  - 读取：`hostnamectl status` 获取主机名、操作系统、内核、架构、硬件信息等
  - 设置主机名：`hostnamectl set-hostname <hostname>`
  - 优点：统一工具，一次获取所有基本系统信息，信息完整
  - 可获取的信息包括：
    - Static hostname（静态主机名）
    - Pretty hostname（漂亮主机名）
    - Icon name（图标名称）
    - Chassis（机箱类型）
    - Machine ID（机器ID）
    - Boot ID（启动ID）
    - Operating System（操作系统）
    - Kernel（内核版本）
    - Architecture（架构）
    - Hardware Vendor（硬件厂商）
    - Hardware Model（硬件型号）
- **日期时间和时区**: 统一使用 `timedatectl` 命令
  - 读取：`timedatectl status` 获取本地时间、UTC时间、时区、NTP状态等
  - 设置日期时间：`timedatectl set-time "YYYY-MM-DD HH:MM:SS"`
  - 设置时区：`timedatectl set-timezone <timezone>`
  - 列出时区：`timedatectl list-timezones`
  - 优点：统一工具，同时获取日期时间和时区信息，支持 NTP 同步状态
- **CPU 占用率**: 读取 `/proc/stat` 计算
- **内存占用率**: 读取 `/proc/meminfo` 计算
- **启动时长**: 读取 `/proc/uptime`
- **以太网和 Wi-Fi 状态**: 统一使用 `nmcli` (NetworkManager) 命令
  - 读取设备状态：`nmcli device status`
  - 读取连接信息：`nmcli connection show`
  - 读取设备详细信息：`nmcli device show <interface>`
  - 优点：统一工具，同时管理以太网和 Wi-Fi，信息完整，易于配置

### 2. 网络配置模块

#### 2.1 以太网配置
- 查看当前配置（IP、子网掩码、网关、DNS）
- 设置静态 IP 或 DHCP
- 修改网络配置（需要 root 权限）

#### 2.2 Wi-Fi 配置
- 扫描可用 Wi-Fi 网络
- 查看已保存的网络配置
- 连接 Wi-Fi（输入 SSID 和密码）
- 断开 Wi-Fi 连接

#### 2.3 实现方式
- **统一使用 NetworkManager (`nmcli`)**
  - 优点：
    - 统一工具：同时管理以太网和 Wi-Fi
    - 现代标准：Linux 系统推荐的网络管理工具
    - 功能完整：支持 DHCP、静态 IP、Wi-Fi 扫描、连接等
    - 易于使用：命令行接口清晰，易于解析
    - 配置持久化：自动保存配置到 `/etc/NetworkManager/system-connections/`
    - 状态监控：可以实时获取连接状态、IP 地址、信号强度等
  - 缺点：
    - 需要 NetworkManager 服务运行（树莓派 OS 通常默认安装并运行）
    - 某些最小化系统可能没有安装
  - 备选方案：直接修改配置文件（`/etc/NetworkManager/system-connections/` 或 `/etc/netplan/`）

### 3. HAT-40Pin 控制模块

#### 3.1 GPIO 控制
- 查看 40pin GPIO 引脚状态
- 设置 GPIO 引脚为输入/输出模式
- 设置 GPIO 输出电平（HIGH/LOW）
- 读取 GPIO 输入电平
- **限制**: 仅支持以下 GPIO 引脚设置输出电平：
  - GPIO 5, 6, 16, 17, 22, 23, 24, 25, 26, 27
  - 其他引脚有功能复用，不能设置为 GPIO 输出模式

#### 3.2 PWM 控制
- 设置 PWM 频率和占空比
- 实时调整 PWM 参数
- **限制**: 仅支持硬件 PWM：
  - GPIO 12 (PWM0)
  - GPIO 13 (PWM1)
  - 不支持软件 PWM

#### 3.3 SPI 通信
- 配置 SPI 参数（模式、速度、位宽）
- SPI 数据发送和接收
- 实时显示通信数据（十六进制和ASCII）
- **限制**: 仅支持以下 SPI 设备：
  - `/dev/spidev0.0` (SPI0, CS0)
  - `/dev/spidev0.1` (SPI0, CS1)

#### 3.4 I2C 通信
- 扫描 I2C 总线上的设备
- 读取/写入 I2C 设备寄存器
- 显示设备地址和通信数据
- **限制**: 仅支持以下 I2C 总线：
  - `/dev/i2c-1` (I2C1)

#### 3.5 UART 通信
- 配置串口参数（波特率、数据位、停止位、校验位）
- 串口数据发送和接收
- 实时显示收发数据（十六进制和ASCII）
- **限制**: 仅支持以下 UART 设备：
  - `/dev/ttyAMA0` (UART0)

#### 3.6 实现方式

**GPIO 控制**
- **推荐方案**: 使用 `libgpiod` 库
  - 优点：
    - 现代标准：使用 GPIO 字符设备接口（Linux 4.8+），替代已弃用的 sysfs 接口
    - 更高效：基于 ioctl 的直接内核交互，性能更好
    - 更可靠：自动资源管理，关闭文件描述符时自动释放资源
    - 功能丰富：支持事件轮询、批量操作、开漏/开集输出等
    - 官方推荐：Linux 内核推荐的 GPIO 操作方式
    - 跨平台：支持所有使用 GPIO 字符设备的 Linux 系统
  - 缺点：
    - 需要额外依赖库（但树莓派 OS 通常已预装）
    - API 相对复杂一些（但提供了更好的抽象）
  - 备选方案：`/sys/class/gpio/` 文件系统接口（已弃用，不推荐）

**PWM 控制**
- **推荐方案**: 使用 `/sys/class/pwm/` 文件系统接口（树莓派内核支持）
  - 优点：
    - 标准 Linux 接口，无需额外依赖
    - 实现简单，通过文件读写操作
    - 跨平台兼容，不限于树莓派
    - 无守护进程，资源占用小
  - 缺点：
    - 性能相对较低（文件 I/O），但满足监控软件需求
    - 功能有限（仅基本设置），但满足项目需求
  - 备选方案：`pigpio` 库（如果未来需要高级功能如软件 PWM、波形生成等）

**SPI 通信**
- 使用 `/dev/spidev*` 设备文件
- 通过 `ioctl` 系统调用配置和传输数据
- 需要 `linux/spi/spidev.h` 头文件

**I2C 通信**
- 使用 `/dev/i2c-*` 设备文件
- 通过 `ioctl` 系统调用进行读写操作
- 需要 `linux/i2c-dev.h` 头文件
- 可选：使用 `i2c-tools` 命令行工具进行测试

**UART 通信**
- 使用 `/dev/tty*` 设备文件
- 通过标准 POSIX 串口 API（`termios.h`）配置参数
- 使用 `read()`/`write()` 进行数据收发
- **使用 WebSocket 提供实时双向数据流**（UART 是唯一使用 WebSocket 的模块）

**权限要求**
- 统一使用 `piduier` 用户组访问所有硬件接口
- 通过 udev 规则配置，让 `piduier` 组拥有所有硬件设备的访问权限
- 详细配置说明见 README.md

## API 设计

### 监控 API

```
GET /api/system/info
返回: {
  "system": {
    "static_hostname": "raspberrypi",
    "pretty_hostname": "Raspberry Pi",
    "icon_name": "computer",
    "chassis": "embedded",
    "machine_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
    "boot_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
    "operating_system": "Raspberry Pi OS GNU/Linux",
    "kernel": "Linux 6.1.0-rpi7-rpi-v8",
    "architecture": "aarch64",
    "hardware_vendor": "Raspberry Pi Foundation",
    "hardware_model": "Raspberry Pi 4 Model B"
  },
  "datetime": {
    "local": "2026-01-13 16:59:51 CST",
    "utc": "2026-01-13 08:59:51 UTC",
    "timezone": "Asia/Shanghai",
    "timezone_offset": "+0800",
    "ntp_synchronized": true,
    "ntp_service": "active"
  },
  "cpu_usage": 15.5,
  "memory": {
    "total": 4096,
    "used": 2048,
    "free": 2048,
    "usage_percent": 50.0
  },
  "uptime": 86400,
  "uptime_formatted": "1 day, 0:00:00",
  "network": {
    "devices": [
      {
        "device": "eth0",
        "type": "ethernet",
        "state": "connected",
        "connection": "Wired connection 1",
        "ip": "192.168.1.100",
        "mac": "b8:27:eb:xx:xx:xx",
        "speed": "1000"
      },
      {
        "device": "wlan0",
        "type": "wifi",
        "state": "connected",
        "connection": "MyWiFi",
        "ip": "192.168.1.101",
        "mac": "dc:a6:32:xx:xx:xx",
        "ssid": "MyWiFi",
        "signal": -45
      }
    ]
  }
}
```

### 日期时间和时区 API

```
GET /api/system/datetime
返回: {
  "local": "2026-01-13 16:59:51 CST",
  "utc": "2026-01-13 08:59:51 UTC",
  "timezone": "Asia/Shanghai",
  "timezone_offset": "+0800",
  "ntp_synchronized": true,
  "ntp_service": "active"
}

POST /api/system/datetime
请求体: {
  "datetime": "2025-01-04 17:30:00"  // 格式: "YYYY-MM-DD HH:MM:SS"
}
注意: 需要 root 权限

POST /api/system/timezone
请求体: {
  "timezone": "Asia/Shanghai"  // 时区名称，如 "Asia/Shanghai", "UTC" 等
}
注意: 需要 root 权限

GET /api/system/timezones
返回: 可用时区列表
[
  "Africa/Abidjan",
  "Africa/Accra",
  ...
  "Asia/Shanghai",
  ...
]
```

### 系统信息 API

```
GET /api/system/hostname
返回: {
  "static_hostname": "raspberrypi",
  "pretty_hostname": "Raspberry Pi",
  "icon_name": "computer",
  "chassis": "embedded",
  "machine_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "boot_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "operating_system": "Raspberry Pi OS GNU/Linux",
  "kernel": "Linux 6.1.0-rpi7-rpi-v8",
  "architecture": "aarch64",
  "hardware_vendor": "Raspberry Pi Foundation",
  "hardware_model": "Raspberry Pi 4 Model B"
}

POST /api/system/hostname
请求体: {
  "hostname": "new-hostname"  // 设置静态主机名
}
注意: 需要 root 权限

POST /api/system/hostname/pretty
请求体: {
  "pretty_hostname": "My Raspberry Pi"  // 设置漂亮主机名（可选）
}
注意: 需要 root 权限
```

### 网络配置 API（使用 NetworkManager）

```
GET /api/network/devices
返回: 所有网络设备状态列表
[
  {
    "device": "eth0",
    "type": "ethernet",
    "state": "connected",
    "connection": "Wired connection 1",
    "ip": "192.168.1.100",
    "mac": "b8:27:eb:xx:xx:xx",
    "speed": "1000"
  },
  {
    "device": "wlan0",
    "type": "wifi",
    "state": "connected",
    "connection": "MyWiFi",
    "ip": "192.168.1.101",
    "mac": "dc:a6:32:xx:xx:xx",
    "ssid": "MyWiFi",
    "signal": -45
  }
]

GET /api/network/device/{interface}
返回: 指定设备的详细信息
注意: interface 可以是 "eth0", "wlan0" 等

GET /api/network/connection/{name}
返回: 指定连接的配置信息

POST /api/network/ethernet/config
请求体: {
  "interface": "eth0",  // 可选，默认 eth0
  "mode": "static|dhcp",
  "ip": "192.168.1.100",
  "netmask": "255.255.255.0",
  "gateway": "192.168.1.1",
  "dns": ["8.8.8.8", "8.8.4.4"]
}
注意: 使用 nmcli connection modify 或 nmcli connection add

GET /api/network/wifi/scan
返回: 可用 Wi-Fi 网络列表
[
  {
    "ssid": "MyWiFi",
    "bssid": "aa:bb:cc:dd:ee:ff",
    "signal": -45,
    "security": "WPA2",
    "channel": 6
  },
  ...
]
注意: 使用 nmcli device wifi list

POST /api/network/wifi/connect
请求体: {
  "ssid": "MyWiFi",
  "password": "mypassword"
}
注意: 使用 nmcli device wifi connect

POST /api/network/wifi/disconnect
请求体: {
  "interface": "wlan0"  // 可选
}
注意: 使用 nmcli device disconnect

GET /api/network/connections
返回: 所有已保存的连接配置列表
```

### GPIO 控制 API

```
GET /api/gpio/status
返回: 所有 GPIO 引脚状态（仅显示可配置的引脚：5,6,16,17,22,23,24,25,26,27）

GET /api/gpio/{pin}
返回: 指定引脚状态
注意: pin 必须是 5, 6, 16, 17, 22, 23, 24, 25, 26, 27 之一

POST /api/gpio/{pin}/output
请求体: {
  "value": 0|1  // 0=LOW, 1=HIGH
}
注意: 仅支持 GPIO 5, 6, 16, 17, 22, 23, 24, 25, 26, 27

POST /api/gpio/{pin}/mode
请求体: {
  "mode": "input|output"
}
注意: 仅支持 GPIO 5, 6, 16, 17, 22, 23, 24, 25, 26, 27
```

### PWM 控制 API

```
GET /api/pwm/channels
返回: 可用 PWM 通道列表（PWM0=GPIO12, PWM1=GPIO13）

GET /api/pwm/{channel}
返回: 指定通道的配置和状态
注意: channel 必须是 "0" (GPIO12) 或 "1" (GPIO13)

POST /api/pwm/{channel}/config
请求体: {
  "frequency": 1000,  // Hz
  "duty_cycle": 50.0  // 百分比 0-100
}
注意: 仅支持 PWM0 (GPIO12) 和 PWM1 (GPIO13)

POST /api/pwm/{channel}/enable
请求体: {
  "enabled": true|false
}
注意: 仅支持 PWM0 (GPIO12) 和 PWM1 (GPIO13)
```

### SPI 通信 API

```
GET /api/spi/devices
返回: 可用 SPI 设备列表（spidev0.0, spidev0.1）

POST /api/spi/{device}/config
请求体: {
  "mode": 0|1|2|3,
  "speed": 1000000,  // Hz
  "bits_per_word": 8
}
注意: device 必须是 "spidev0.0" 或 "spidev0.1"

POST /api/spi/{device}/transfer
请求体: {
  "data": [0x01, 0x02, 0x03]  // 字节数组
}
返回: {
  "data": [0xFF, 0xFE, 0xFD]  // 接收到的数据
}
注意: 仅支持 /dev/spidev0.0 和 /dev/spidev0.1
```

### I2C 通信 API

```
GET /api/i2c/buses
返回: 可用 I2C 总线列表（仅 i2c-1）

GET /api/i2c/{bus}/scan
返回: 扫描到的设备地址列表
注意: bus 必须是 "1" (对应 /dev/i2c-1)

POST /api/i2c/{bus}/{address}/read
请求体: {
  "register": 0x00,  // 可选，寄存器地址
  "length": 1        // 读取字节数
}
返回: {
  "data": [0xFF]
}
注意: 仅支持 bus="1" (/dev/i2c-1)

POST /api/i2c/{bus}/{address}/write
请求体: {
  "register": 0x00,  // 可选，寄存器地址
  "data": [0x01, 0x02]
}
注意: 仅支持 bus="1" (/dev/i2c-1)
```

### UART 通信 API

```
GET /api/uart/devices
返回: 可用 UART 设备列表（仅 ttyAMA0）

GET /api/uart/{device}/config
返回: 当前串口配置
注意: device 必须是 "ttyAMA0" (对应 /dev/ttyAMA0)

POST /api/uart/{device}/config
请求体: {
  "baudrate": 115200,
  "data_bits": 8,
  "stop_bits": 1,
  "parity": "none|even|odd",
  "flow_control": "none|rtscts"
}
注意: 仅支持 device="ttyAMA0" (/dev/ttyAMA0)

POST /api/uart/{device}/send
请求体: {
  "data": "Hello World"  // 字符串或字节数组
}
注意: 仅支持 device="ttyAMA0"

GET /api/uart/{device}/receive
返回: 接收到的数据（轮询方式）
注意: 仅支持 device="ttyAMA0"

// WebSocket 方式（UART 专用，提供实时双向数据流）
WS /ws/uart/ttyAMA0
双向通信：发送和接收数据
- 客户端发送: UART 发送数据（二进制或文本）
- 服务器推送: UART 接收数据（二进制或文本）
注意: 仅支持 ttyAMA0，这是唯一使用 WebSocket 的模块
```

## 前端界面设计

### 设计规范

#### 色彩方案（OKLCH 色彩空间）
- **主色调（强调色）**: `oklch(62% 0.214 260)` - 蓝色，用于主要操作和强调
- **常规色（成功）**: `oklch(65% 0.15 145)` - 绿色，表示正常状态
- **警告色**: `oklch(75% 0.15 85)` - 黄色，表示警告
- **错误色**: `oklch(55% 0.2 25)` - 红色，表示错误
- **背景色**: `oklch(20% 0.02 260)` - 深色背景
- **卡片背景**: `oklch(25% 0.02 260)` - 稍亮的卡片背景
- **文字主色**: `oklch(85% 0.01 260)` - 主要文字
- **文字次色**: `oklch(60% 0.01 260)` - 次要文字
- **边框色**: `oklch(30% 0.02 260)` - 边框和分隔线

#### 布局结构
- **桌面端**: 左侧固定侧边栏（宽度 280px），右侧内容区域（自适应）
- **移动端**: 汉堡菜单按钮，侧边栏可折叠/展开
- **响应式断点**: 
  - 移动端: < 768px
  - 平板: 768px - 1024px
  - 桌面: > 1024px

### 页面结构

#### 1. 整体布局
```
┌─────────────────────────────────────────┐
│ 顶部栏（高度 60px）                      │
│ [Logo] 树莓派监控系统    [刷新] [设置]   │
├──────────┬──────────────────────────────┤
│          │                              │
│ 侧边栏   │  主内容区域                  │
│ (280px)  │  (自适应)                    │
│          │                              │
│ 菜单树   │  - 监控面板                  │
│          │  - 网络设置                  │
│          │  - HAT-40Pin                 │
│          │    ├─ GPIO                   │
│          │    ├─ UART                   │
│          │    ├─ SPI                    │
│          │    ├─ I2C                    │
│          │    └─ PWM                    │
│          │  - 关于                      │
│          │                              │
└──────────┴──────────────────────────────┘
```

#### 2. 监控面板页面
- **系统信息卡片组**（网格布局，响应式）
  - 卡片 1: Hostname、日期时间、时区
  - 卡片 2: CPU 占用率（圆形进度条 + 百分比）
  - 卡片 3: 内存占用率（进度条 + 已用/总计）
  - 卡片 4: 启动时长（格式化显示）
  
- **网络状态卡片组**
  - 卡片 5: 以太网状态
    - 连接状态指示器（绿色/红色圆点）
    - IP 地址、MAC 地址、速度
  - 卡片 6: Wi-Fi 状态
    - 连接状态指示器
    - SSID、IP 地址、信号强度（信号图标）

- **实时更新**: 每 2 秒自动刷新数据

#### 3. 网络设置页面
- **标签页切换**: 以太网 | Wi-Fi
- **以太网标签页**:
  - 当前配置信息卡片
  - 配置表单（卡片样式）
    - 模式选择：静态 IP / DHCP（切换开关）
    - IP 地址输入框
    - 子网掩码输入框
    - 网关输入框
    - DNS 服务器输入框（可添加多个）
  - 操作按钮：保存、重置
  
- **Wi-Fi 标签页**:
  - 扫描按钮（主要操作按钮，蓝色）
  - 可用网络列表（卡片列表）
    - 每个网络显示：SSID、信号强度、加密类型、连接按钮
  - 已保存网络列表
    - 显示已配置的网络，可删除或重新连接

#### 4. HAT-40Pin 子菜单

##### 4.1 GPIO 页面
- **40pin 引脚布局图**（可视化）
  - 物理引脚排列（2列，每列20个）
  - 每个引脚显示：
    - 引脚号（物理编号）
    - GPIO 号（如果可用）
    - 当前功能（GPIO/SPI/I2C/UART/PWM）
    - 状态指示（输入/输出、电平高低）
  - **可配置引脚高亮显示**：
    - GPIO 5, 6, 16, 17, 22, 23, 24, 25, 26, 27 可点击配置
    - 其他引脚显示为灰色（不可配置），并显示其功能复用信息
  - 点击可配置引脚弹出配置对话框：
    - 模式选择（输入/输出）
    - 输出电平设置（HIGH/LOW）
    - 实时状态显示

##### 4.2 UART 页面
- **左侧：配置面板**
  - 设备显示：ttyAMA0（固定，不可选择）
  - 参数配置：
    - 波特率（常用值快速选择 + 自定义）
    - 数据位（5/6/7/8）
    - 停止位（1/2）
    - 校验位（无/奇/偶）
    - 流控制（无/RTS-CTS）
  - 连接/断开按钮
  
- **右侧：数据收发区域**
  - 接收区域（上半部分）
    - 数据格式切换：ASCII / 十六进制
    - 接收数据显示区（可滚动，自动滚动到底部）
    - 清空按钮
    - 保存日志按钮
  - 发送区域（下半部分）
    - 数据格式切换：ASCII / 十六进制
    - 输入框（多行文本）
    - 发送按钮
    - 自动发送（定时发送）开关

##### 4.3 SPI 页面
- **配置区域**
  - 设备选择（spidev0.0, spidev0.1，仅此两个选项）
  - SPI 模式（0/1/2/3）
  - 速度设置（Hz，带常用值快速选择）
  - 位宽（8/16）
  - 应用配置按钮
  
- **测试区域**
  - 发送数据输入（十六进制或字节数组）
  - 接收数据显示
  - 发送按钮
  - 连续发送模式（带间隔设置）

##### 4.4 I2C 页面
- **总线显示**: I2C1（固定，不可选择）
- **设备扫描区域**
  - 扫描按钮
  - 扫描结果列表（显示设备地址，7位和8位格式）
  - 点击设备地址可查看详细信息
  
- **设备操作区域**
  - 选择设备地址
  - 寄存器地址输入（可选）
  - 读取操作：
    - 读取长度设置
    - 读取按钮
    - 显示读取结果（十六进制和ASCII）
  - 写入操作：
    - 数据输入（十六进制）
    - 写入按钮

##### 4.5 PWM 页面
- **通道选择**: PWM0 (GPIO12) 和 PWM1 (GPIO13)，两个通道卡片
- **每个通道配置区域**
  - 通道标识（PWM0/PWM1 和对应的 GPIO 号）
  - 频率设置（Hz，带常用值快速选择）
  - 占空比设置（0-100%，滑块 + 数字输入）
  - 启用/禁用开关
  - 实时预览波形图（可选）
  
- **双通道管理**
  - 两个通道独立配置和显示
  - 每个通道的状态独立显示

#### 5. 关于页面
- 版本信息
- 系统信息
- 项目链接
- 许可证信息

### 技术实现

#### 前端技术栈
- **基础**: HTML5 + CSS3 + 原生 JavaScript（无框架依赖）
- **CSS**: 使用 CSS Grid 和 Flexbox 布局
- **色彩**: 使用 OKLCH 色彩空间（现代浏览器支持，回退到 RGB）
- **图标**: 使用 SVG 图标或图标字体
- **实时通信**: 
  - 轮询：`setInterval` 定期调用 REST API
  - WebSocket：用于 UART 实时数据流（UART 专用）

#### UI 组件设计
- **卡片组件**: 圆角、阴影、边框
- **按钮**: 主要按钮（蓝色）、次要按钮（灰色）、危险按钮（红色）
- **输入框**: 深色背景、蓝色焦点边框
- **下拉菜单**: 深色主题，悬停高亮
- **模态对话框**: 半透明背景遮罩，居中显示
- **进度条**: 圆形（CPU/内存）和线性两种样式
- **状态指示器**: 彩色圆点（绿色=正常，黄色=警告，红色=错误）

#### 响应式设计
- **移动端**:
  - 汉堡菜单按钮（左上角）
  - 侧边栏覆盖式显示（从左侧滑入）
  - 卡片改为单列布局
  - 触摸友好的按钮尺寸（最小 44x44px）
  
- **桌面端**:
  - 固定侧边栏
  - 多列网格布局
  - 悬停效果

#### 交互设计
- **动画**: 使用 CSS transitions（0.2s ease）
- **反馈**: 按钮点击反馈、加载状态、成功/错误提示
- **键盘导航**: 支持 Tab 键导航，Enter 键确认
- **无障碍**: 适当的 ARIA 标签和语义化 HTML

## 技术架构

### 后端（C）

```
main.c
├── 系统监控函数
│   ├── get_system_info()
│   ├── get_cpu_usage()
│   ├── get_memory_usage()
│   ├── get_network_status()
│   └── get_wifi_status()
├── 网络配置函数（使用 NetworkManager）
│   ├── get_network_devices()        // nmcli device status
│   ├── get_device_info()           // nmcli device show
│   ├── get_connection_info()       // nmcli connection show
│   ├── get_ethernet_config()       // nmcli connection show <name>
│   ├── set_ethernet_config()       // nmcli connection modify/add
│   ├── scan_wifi()                 // nmcli device wifi list
│   ├── connect_wifi()              // nmcli device wifi connect
│   ├── disconnect_wifi()           // nmcli device disconnect
│   └── list_connections()          // nmcli connection show
├── GPIO 控制函数
│   ├── gpio_export()
│   ├── gpio_set_direction()
│   ├── gpio_set_value()
│   └── gpio_get_value()
├── PWM 控制函数
│   ├── pwm_export()
│   ├── pwm_set_period()
│   ├── pwm_set_duty_cycle()
│   └── pwm_enable()
├── SPI 通信函数
│   ├── spi_open()
│   ├── spi_config()
│   └── spi_transfer()
├── I2C 通信函数
│   ├── i2c_open()
│   ├── i2c_scan()
│   ├── i2c_read()
│   └── i2c_write()
├── UART 通信函数
│   ├── uart_open()
│   ├── uart_config()
│   ├── uart_send()
│   └── uart_receive()
└── HTTP 事件处理
    └── ev_handler() - 处理所有 API 请求
    └── WebSocket 处理（仅用于 UART 实时数据流）
```

### 文件结构

```
piduier/
├── docs/                          # 文档目录
│   ├── IMPLEMENTATION_PLAN.md     # 总体实现方案（本文件）
│   ├── GPIO_IMPLEMENTATION.md     # GPIO 实现方案
│   ├── PWM_IMPLEMENTATION.md      # PWM 实现方案
│   ├── CPU_USAGE_IMPLEMENTATION.md # CPU 占用率实现方案
│   ├── PERMISSIONS_IMPLEMENTATION.md # 权限管理实现方案
│   ├── WEBSOCKET_IMPLEMENTATION.md # WebSocket 实现方案
│   ├── NETWORKMANAGER_IMPLEMENTATION.md # NetworkManager 实现方案
│   ├── HOSTNAMECTL_IMPLEMENTATION.md # hostnamectl 实现方案
│   ├── TIMEDATECTL_IMPLEMENTATION.md # timedatectl 实现方案
│   └── FILE_STRUCTURE.md          # 文件结构设计文档
│
├── src/                           # 源代码目录
│   ├── main.c                     # 程序入口，HTTP 服务器初始化
│   ├── http/                      # HTTP 处理模块
│   │   ├── router.c/h            # 路由处理，API 端点分发
│   │   └── websocket.c/h         # WebSocket 处理（UART 专用）
│   ├── system/                    # 系统监控模块
│   │   ├── system_info.c/h       # 系统信息（hostnamectl）
│   │   ├── datetime.c/h          # 日期时间（timedatectl）
│   │   ├── cpu.c/h               # CPU 占用率计算
│   │   ├── memory.c/h            # 内存使用率
│   │   └── uptime.c/h            # 系统运行时间
│   ├── network/                   # 网络配置模块
│   │   ├── network_manager.c/h   # NetworkManager 接口封装
│   │   ├── ethernet.c/h           # 以太网配置
│   │   └── wifi.c/h              # Wi-Fi 配置
│   └── hardware/                  # 硬件控制模块
│       ├── gpio.c/h              # GPIO 控制（libgpiod）
│       ├── pwm.c/h               # PWM 控制
│       ├── spi.c/h                # SPI 通信
│       ├── i2c.c/h                # I2C 通信
│       └── uart.c/h               # UART 通信
│
├── web/                           # Web 前端目录
│   └── index.html                 # 主页面
│
├── mongoose.c                     # Mongoose 库源文件
├── mongoose.h                     # Mongoose 库头文件
│
├── CMakeLists.txt                 # CMake 构建配置
├── build.sh                       # 构建脚本
└── README.md                      # 项目说明文档
```

**设计原则**：
- **模块化**: 按功能划分模块，每个模块独立目录
- **可扩展**: 预留扩展空间，便于添加新功能
- **可维护**: 文档集中管理，源码按模块组织
- **清晰**: 目录结构清晰，便于定位代码

详细说明见 `docs/FILE_STRUCTURE.md`

## 安全考虑

1. **权限管理**
   - 所有修改操作需要 root 权限
   - 考虑添加简单的认证机制（HTTP Basic Auth 或 Token）

2. **输入验证**
   - 验证所有用户输入（IP 地址格式、GPIO 引脚号等）
   - 防止命令注入攻击

3. **错误处理**
   - 完善的错误处理和错误信息返回
   - 操作失败时给出明确提示

## 实现步骤

### 阶段 1: 系统监控（基础功能）✅ 已完成
1. ✅ 实现系统信息获取 API
   - ✅ 系统信息（hostnamectl）
   - ✅ 日期时间（timedatectl）
   - ✅ CPU 占用率（/proc/stat，带缓存）
   - ✅ 内存占用率（/proc/meminfo）
   - ✅ 系统运行时间（/proc/uptime）
   - ✅ CPU 温度（/sys/class/thermal/thermal_zone0/temp）
   - ✅ 存储占用率（df 命令，支持 /dev/mmcblk0p2）
   - ✅ 网络设备状态（nmcli，支持以太网和 Wi-Fi）
   - ✅ 综合系统信息 API（/api/system/info/complete）
2. ✅ 创建基础 Web 界面显示系统信息
   - ✅ 现代工业风格深色主题 UI
   - ✅ 响应式布局（桌面侧边栏、移动端汉堡菜单）
   - ✅ 树形菜单结构
   - ✅ 实时数据更新（HTTP 轮询）
3. ✅ 实现自动刷新功能
4. ✅ 系统控制功能
   - ✅ 系统重启（/api/system/reboot）
   - ✅ 系统关闭（/api/system/shutdown）

### 阶段 2: 网络配置
1. 实现以太网配置读取和设置
2. 实现 Wi-Fi 扫描和连接功能
3. 添加网络配置界面

### 阶段 3: HAT-40Pin 控制
1. **GPIO 控制**
   - 实现 GPIO 操作函数
   - 创建 GPIO 控制界面（40pin 布局图）
   - 添加引脚状态实时更新

2. **PWM 控制**
   - 实现 PWM 操作函数
   - 创建 PWM 配置界面
   - 支持多通道管理

3. **SPI 通信**
   - 实现 SPI 通信函数
   - 创建 SPI 配置和测试界面
   - 数据收发功能

4. **I2C 通信**
   - 实现 I2C 通信函数
   - 创建 I2C 扫描和操作界面
   - 寄存器读写功能

5. **UART 通信**
   - 实现 UART 通信函数
   - 创建 UART 配置和数据收发界面
   - 支持 WebSocket 实时数据流（UART 专用）

### 阶段 4: 优化和增强
1. 添加历史数据记录（可选）
2. 优化界面和用户体验
3. 添加安全认证（可选）

## 依赖和限制

### 系统依赖
- Linux 系统（树莓派 OS，内核 4.8+）
- **NetworkManager**（网络管理，树莓派 OS 默认安装）
  - 服务：`systemctl status NetworkManager`
  - 命令行工具：`nmcli`（通常已预装）
  - 如果未安装：`sudo apt-get install network-manager`
- **libgpiod** 库（GPIO 控制）
  - 安装：`sudo apt-get install libgpiod-dev`
  - 链接：编译时需要链接 `-lgpiod`
- **PWM 支持**：树莓派内核需要启用 PWM（通常默认启用）
- **SPI/I2C/UART 支持**：需要通过 `raspi-config` 启用相应接口

### 权限要求
- 统一使用 `piduier` 用户组访问所有硬件接口（GPIO、PWM、SPI、I2C、UART）
- 网络配置操作需要 root 权限（通过 NetworkManager）
- 详细配置说明见 README.md

### 硬件接口限制

#### GPIO 限制
- **可配置输出电平的引脚**: 仅 GPIO 5, 6, 16, 17, 22, 23, 24, 25, 26, 27
- 其他引脚有功能复用（电源、地、I2C、SPI、UART等），不能设置为 GPIO 输出模式
- 需要 root 权限操作 GPIO

#### PWM 限制
- **支持的通道**: 仅 PWM0 (GPIO12) 和 PWM1 (GPIO13)
- 仅支持硬件 PWM，不支持软件 PWM
- 需要 root 权限操作 PWM

#### SPI 限制
- **支持的设备**: 仅 `/dev/spidev0.0` 和 `/dev/spidev0.1`
- 需要启用 SPI 接口（raspi-config）
- 需要 root 权限访问 SPI 设备

#### I2C 限制
- **支持的总线**: 仅 `/dev/i2c-1` (I2C1)
- 需要启用 I2C 接口（raspi-config）
- 需要 root 权限访问 I2C 设备

#### UART 限制
- **支持的设备**: 仅 `/dev/ttyAMA0` (UART0)
- 需要启用串口（raspi-config），注意 UART0 可能被蓝牙占用
- 需要 root 权限访问串口设备

#### 通用限制
- 所有硬件操作需要 root 权限
- 建议在界面上明确显示可用的硬件接口和限制
- 添加引脚功能说明和警告提示

## 问题与考虑

1. **网络配置方式选择**
   - NetworkManager (nmcli) vs 传统配置文件方式
   - 建议：优先使用 NetworkManager，更现代且易于管理

2. **GPIO 库选择**
   - **libgpiod**（推荐）：现代标准，Linux 内核推荐，性能好，功能丰富
   - `/sys/class/gpio/`：已弃用，不推荐使用
   - wiringPi：已停止维护，不推荐
   - pigpio：功能丰富但主要用于远程控制，本地操作推荐 libgpiod
   - **建议：使用 libgpiod**，这是 Linux 内核推荐的 GPIO 操作方式

3. **实时更新频率**
   - 建议：系统信息 1-2 秒更新一次，避免过于频繁

4. **数据持久化**
   - GPIO 配置是否需要保存？
   - 网络配置需要持久化到配置文件

## 后续扩展（可选）

1. 系统日志查看
2. 温度监控（CPU、GPU 温度）
3. 磁盘使用情况
4. 进程管理
5. 系统服务管理
6. 历史数据图表（CPU/内存趋势）
7. 告警功能（阈值告警）
8. 数据导出功能（UART/SPI/I2C 日志）
9. 预设配置保存和加载
10. 多语言支持
