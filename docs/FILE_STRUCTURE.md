# 文件结构设计

## 目录结构

```
piduier/
├── docs/                          # 文档目录
│   ├── IMPLEMENTATION_PLAN.md     # 总体实现方案
│   ├── GPIO_IMPLEMENTATION.md     # GPIO 实现方案
│   ├── PWM_IMPLEMENTATION.md      # PWM 实现方案
│   ├── CPU_USAGE_IMPLEMENTATION.md # CPU 占用率实现方案
│   ├── PERMISSIONS_IMPLEMENTATION.md # 权限管理实现方案
│   ├── WEBSOCKET_IMPLEMENTATION.md # WebSocket 实现方案
│   ├── NETWORKMANAGER_IMPLEMENTATION.md # NetworkManager 实现方案
│   ├── HOSTNAMECTL_IMPLEMENTATION.md # hostnamectl 实现方案
│   ├── TIMEDATECTL_IMPLEMENTATION.md # timedatectl 实现方案
│   └── FILE_STRUCTURE.md          # 文件结构设计文档（本文件）
│
├── src/                           # 源代码目录
│   ├── main.c                     # 程序入口，HTTP 服务器初始化
│   ├── http/                      # HTTP 处理模块
│   │   ├── router.c/h            # 路由处理，API 端点分发
│   │   └── websocket.c/h          # WebSocket 处理（UART 专用）
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
│   ├── index.html                 # 主页面（HTML 结构）
│   ├── css/                       # 样式文件
│   │   └── style.css              # 所有样式（OKLCH 色彩、响应式布局）
│   └── js/                        # JavaScript 文件
│       └── app.js                 # 所有 JavaScript 逻辑（API 调用、UI 更新）
│
├── mongoose.c                     # Mongoose 库源文件
├── mongoose.h                     # Mongoose 库头文件
│
├── CMakeLists.txt                 # CMake 构建配置
├── build.sh                       # 构建脚本
├── README.md                      # 项目说明文档
└── .gitignore                     # Git 忽略文件
```

## 设计原则

### 1. 模块化设计
- **按功能划分模块**：系统监控、网络配置、硬件控制各自独立
- **每个模块独立目录**：便于单独测试和维护
- **清晰的接口定义**：每个模块提供明确的头文件接口

### 2. 可扩展性
- **预留扩展空间**：每个模块目录可以继续细分
- **统一的接口风格**：便于添加新功能模块
- **松耦合设计**：模块之间通过接口通信，减少依赖

### 3. 可维护性
- **文档集中管理**：所有实现方案文档放在 `docs/` 目录
- **源码按模块组织**：便于定位和修改代码
- **前后端分离**：Web 前端独立目录，便于维护
- **前端代码分离**：HTML、CSS、JavaScript 分离，职责清晰，易于维护

## 模块说明

### HTTP 处理模块 (`src/http/`)
- **router.c/h**: 路由处理，根据 URL 路径分发到相应的处理函数
- **websocket.c/h**: WebSocket 处理，仅用于 UART 实时数据流

### 系统监控模块 (`src/system/`) ✅ 已实现
- ✅ **system_info.c/h**: 系统信息获取（hostnamectl）
- ✅ **datetime.c/h**: 日期时间管理（timedatectl）
- ✅ **cpu.c/h**: CPU 占用率计算（/proc/stat，带后台缓存）
- ✅ **memory.c/h**: 内存使用率（/proc/meminfo）
- ✅ **uptime.c/h**: 系统运行时间（/proc/uptime）
- ✅ **temperature.c/h**: CPU 温度监控（/sys/class/thermal/thermal_zone0/temp）
- ✅ **storage.c/h**: 存储使用率（df 命令）

### 网络配置模块 (`src/network/`) ⏳ 部分实现
- ✅ **network_manager.c/h**: NetworkManager 接口封装（nmcli 命令封装，状态监控已实现）
- ⏳ **ethernet.c/h**: 以太网配置相关函数（待实现）
- ⏳ **wifi.c/h**: Wi-Fi 配置相关函数（待实现）

### 硬件控制模块 (`src/hardware/`) ⏳ 待实现
- ⏳ **gpio.c/h**: GPIO 控制（libgpiod）
- ⏳ **pwm.c/h**: PWM 控制（/sys/class/pwm/）
- ⏳ **spi.c/h**: SPI 通信（/dev/spidev*）
- ⏳ **i2c.c/h**: I2C 通信（/dev/i2c-*）
- ⏳ **uart.c/h**: UART 通信（/dev/tty*）

## 编译配置

### CMakeLists.txt 结构
```cmake
# 主程序
add_executable(piduier src/main.c mongoose.c)

# 包含目录
target_include_directories(piduier PRIVATE
    src
    src/http
    src/system
    src/network
    src/hardware
)

# 链接库
target_link_libraries(piduier pthread gpiod)

# 安装规则
install(TARGETS piduier RUNTIME DESTINATION bin)
install(DIRECTORY web/ DESTINATION bin/web)
```

## 未来扩展

### 添加新功能模块
1. 在相应目录下创建新的 `.c/.h` 文件
2. 在 `CMakeLists.txt` 中添加源文件
3. 在 `router.c` 中添加新的路由处理

### 添加新的硬件接口
1. 在 `src/hardware/` 下创建新的接口文件
2. 实现统一的接口函数（open, config, read, write, close）
3. 在 `CMakeLists.txt` 中添加源文件

### 添加新的系统监控项
1. 在 `src/system/` 下创建新的监控文件
2. 实现数据获取函数
3. 在系统信息 API 中集成

## 文件命名规范

- **源文件**: 小写字母，下划线分隔（如 `system_info.c`）
- **头文件**: 与源文件同名（如 `system_info.h`）
- **函数命名**: 模块前缀 + 功能名（如 `system_get_info()`）
- **常量命名**: 全大写，下划线分隔（如 `MAX_BUFFER_SIZE`）

## 构建和运行

### 构建
```bash
./build.sh
```

### 构建产物

编译完成后，会在 `bin/` 目录生成以下文件：

```
bin/
├── piduier          # 可执行文件（主程序）
└── web/             # Web 前端文件目录
    ├── index.html
    ├── css/
    │   └── style.css
    └── js/
        └── app.js
```

**说明**：
- `build/` 目录包含 CMake 生成的中间文件，**不需要部署**
- `bin/` 目录包含需要部署到树莓派的所有文件

### 运行
```bash
# 可执行文件在 bin/ 目录
./bin/piduier

# Web 文件在 bin/web/ 目录
# 服务器会自动从 bin/web/ 目录提供静态文件
```

### 部署到树莓派

只需要将 `bin/` 目录下的所有文件复制到树莓派即可：

```bash
# 编译 ARM64 版本
ARCH=arm64 ./build.sh

# 复制到树莓派
scp -r bin/ pi@raspberrypi.local:/home/pi/piduier/
```

详细部署说明见 `docs/BUILD_AND_DEPLOY.md`
