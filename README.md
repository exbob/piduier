# Hostname 管理 Web 服务

基于 Mongoose 的简单 HTTP 服务器，提供通过 Web 界面管理系统 hostname 的功能。

## 编译

### 方法 1：使用 build.sh 脚本（推荐）

支持交叉编译，通过 ARCH 参数指定架构：

**x86 架构（默认）：**
```bash
./build.sh x86
# 或
./build.sh
```

**ARM64 架构：**
```bash
./build.sh arm64
```

**说明：**
- CMake 生成的所有文件都放在 `./build` 目录下
- 编译后的可执行文件会自动安装到 `./bin/server`

### 方法 2：直接使用 CMake

```bash
# x86 编译
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)
cmake --install . --prefix ..

# ARM64 交叉编译
mkdir -p build && cd build
CC=aarch64-linux-gnu-gcc cmake ..
cmake --build . -j$(nproc)
cmake --install . --prefix ..
```

### 方法 3：使用 Makefile（备选）

```bash
make
```

### 方法 4：直接使用 gcc

```bash
gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -pthread main.c mongoose.c -o server
```

## 运行

**注意**：修改 hostname 需要 root 权限，因此需要使用 `sudo` 运行。

```bash
# 使用 build.sh 编译后（可执行文件在 bin/ 目录）
sudo ./bin/server

# 或使用 Makefile 编译后
sudo ./server
```

## 使用

1. 启动服务器后，在浏览器中访问：`http://localhost:8000`
2. 点击"读取"按钮获取当前 hostname
3. 在输入框中修改 hostname
4. 点击"保存"按钮应用更改

## API 端点

- `GET /api/hostname` - 获取当前 hostname
- `POST /api/hostname` - 设置新的 hostname（请求体：`{"hostname": "新hostname"}`）

## 清理

**清理 CMake 构建文件和安装目录：**
```bash
rm -rf build bin
```

**清理 Makefile 构建文件：**
```bash
make clean
```

## 交叉编译说明

项目支持通过 `ARCH` 变量进行交叉编译：

- **ARCH=x86**：使用 `gcc` 编译，适用于 x86/x86_64 架构
- **ARCH=arm64**：使用 `aarch64-linux-gnu-gcc` 编译，适用于 ARM64 架构

确保已安装相应的交叉编译工具链：
- x86: `gcc`（通常系统自带）
- arm64: `sudo apt-get install gcc-aarch64-linux-gnu`（Ubuntu/Debian）

