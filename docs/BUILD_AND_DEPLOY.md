# 编译和部署说明

## 编译后生成的文件

### 1. 构建目录（`./build/`）

CMake 生成的中间文件和构建文件，**不需要部署到树莓派**：

```
build/
├── CMakeCache.txt              # CMake 缓存
├── CMakeFiles/                 # CMake 生成的文件
├── cmake_install.cmake         # 安装脚本
├── Makefile                    # Makefile
└── piduier                     # 编译中的可执行文件（临时）
```

**说明**：这些文件是编译过程中的中间产物，编译完成后可以删除。

### 2. 安装目录（`./bin/`）

编译完成后，通过 `cmake --install` 安装的文件，**这些是需要部署到树莓派的文件**：

```
bin/
├── piduier                     # 可执行文件（主程序）
└── web/                        # Web 前端文件目录
    ├── index.html              # 主页面
    ├── css/                    # 样式文件（如果存在）
    │   └── style.css
    └── js/                     # JavaScript 文件（如果存在）
        └── app.js
```

## 需要安装到树莓派的文件

### 必需文件

只需要将 `bin/` 目录下的所有文件复制到树莓派即可：

```
bin/
├── piduier                     # 可执行文件（必需）
└── web/                        # Web 前端目录（必需）
    ├── index.html              # 必需
    ├── css/                    # 如果存在 CSS 文件
    │   └── style.css
    └── js/                     # 如果存在 JS 文件
        └── app.js
```

### 文件说明

1. **`piduier`** - 可执行文件
   - 主程序，HTTP 服务器
   - 需要执行权限（`chmod +x piduier`）
   - 架构必须匹配（ARM64 版本用于树莓派）

2. **`web/` 目录** - Web 前端文件
   - 服务器会从 `web/` 目录提供静态文件
   - 必须与可执行文件在同一目录下
   - 保持目录结构不变

## 部署到树莓派

### 方法 1：直接复制（推荐）

```bash
# 在开发机上编译（ARM64 架构）
ARCH=arm64 ./build.sh

# 将整个 bin/ 目录复制到树莓派
scp -r bin/ pi@raspberrypi.local:/home/pi/piduier/

# 或者使用 rsync（更高效）
rsync -av bin/ pi@raspberrypi.local:/home/pi/piduier/
```

### 方法 2：打包传输

```bash
# 在开发机上打包
ARCH=arm64 ./build.sh
cd bin
tar czf ../piduier-arm64.tar.gz piduier web/

# 传输到树莓派
scp piduier-arm64.tar.gz pi@raspberrypi.local:/home/pi/

# 在树莓派上解压
ssh pi@raspberrypi.local
cd /home/pi
tar xzf piduier-arm64.tar.gz
```

### 方法 3：在树莓派上直接编译

```bash
# 在树莓派上
git clone <repository-url>
cd piduier
./build.sh  # 默认使用本地编译器（ARM64）
```

## 树莓派上的目录结构

部署后，树莓派上的目录结构应该是：

```
/home/pi/piduier/          # 或你选择的其他目录
├── piduier                # 可执行文件
└── web/                   # Web 前端目录
    ├── index.html
    ├── css/
    │   └── style.css
    └── js/
        └── app.js
```

## 运行前准备

### 1. 设置权限

```bash
# 确保可执行文件有执行权限
chmod +x piduier

# 配置用户组权限（如果使用 piduier 组）
sudo usermod -aG piduier $USER
# 重新登录以使组权限生效
```

### 2. 配置 udev 规则（如果使用 piduier 组）

```bash
# 创建 udev 规则文件
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

# 重新加载 udev 规则
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### 3. 运行程序

```bash
# 进入部署目录
cd /home/pi/piduier

# 直接运行（如果已配置 piduier 组权限）
./piduier

# 或使用 root 权限（开发测试，不推荐生产环境）
sudo ./piduier
```

## 文件大小估算

### 可执行文件
- **ARM64 版本**: 约 150-200 KB（未压缩）
- **x86 版本**: 约 150-200 KB（未压缩）

### Web 前端文件
- **index.html**: 约 10-20 KB
- **style.css**: 约 15-30 KB（如果分离）
- **app.js**: 约 30-50 KB（如果分离）
- **总计**: 约 50-100 KB

### 总大小
- **完整部署包**: 约 200-300 KB（非常小，便于传输）

## 系统依赖

树莓派上需要安装的系统依赖（见 README.md）：

```bash
sudo apt-get update
sudo apt-get install -y \
    libgpiod-dev \
    network-manager \
    build-essential \
    cmake \
    pkg-config
```

**注意**：如果只是运行编译好的程序，只需要运行时库：
- `libgpiod`（运行时库，不是开发库）
- `NetworkManager`（如果使用网络配置功能）

## 验证部署

部署后，可以通过以下方式验证：

```bash
# 1. 检查文件是否存在
ls -lh piduier
ls -lh web/index.html

# 2. 检查可执行文件架构
file piduier
# 应该显示: ELF 64-bit LSB pie executable, ARM aarch64, ...

# 3. 检查权限
ls -l piduier
# 应该显示: -rwxr-xr-x ... piduier

# 4. 运行程序
./piduier
# 应该看到服务器启动信息

# 5. 在浏览器中访问
# http://<树莓派IP>:8000
```

## 总结

**需要部署到树莓派的文件**：
- ✅ `bin/piduier` - 可执行文件
- ✅ `bin/web/` - Web 前端目录（包含所有 HTML/CSS/JS 文件）

**不需要部署的文件**：
- ❌ `build/` - 构建中间文件
- ❌ `src/` - 源代码
- ❌ `docs/` - 文档
- ❌ `CMakeLists.txt`、`build.sh` 等构建文件

**部署方式**：直接复制整个 `bin/` 目录到树莓派即可。
