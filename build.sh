#!/bin/bash

# 构建脚本 - 支持交叉编译
# 用法: ./build.sh [ARCH]
# ARCH 可以是: x86 (默认) 或 arm64

set -e

# 获取架构参数，默认为 x86
ARCH=${1:-x86}

# 设置编译器和工具链
if [ "$ARCH" = "arm64" ]; then
    export CC=aarch64-linux-gnu-gcc
    export CXX=aarch64-linux-gnu-g++
    echo "使用 ARM64 交叉编译工具链: $CC"
elif [ "$ARCH" = "x86" ]; then
    export CC=gcc
    export CXX=g++
    echo "使用 x86 编译工具链: $CC"
else
    echo "错误: 不支持的架构 '$ARCH'"
    echo "支持的架构: x86, arm64"
    exit 1
fi

# 检查编译器是否存在
if ! command -v $CC &> /dev/null; then
    echo "错误: 找不到编译器 $CC"
    echo "请安装相应的工具链"
    exit 1
fi

# 统一使用 build 目录
BUILD_DIR="build"

# 创建构建目录
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# 运行 CMake 配置
echo "配置 CMake..."
cmake ..

# 编译
echo "开始编译..."
cmake --build . -j$(nproc)

# 安装到 bin/ 目录
echo "安装可执行文件到 bin/ 目录..."
cmake --install . --prefix ..

# 返回项目根目录
cd ..

echo ""
echo "编译完成！"
echo "CMake 生成文件位置: ./build/"
echo "可执行文件位置: ./bin/server"
echo ""
echo "运行方式:"
if [ "$ARCH" = "arm64" ]; then
    echo "  注意: ARM64 版本需要在 ARM64 设备上运行，或使用 qemu 模拟"
    echo "  sudo ./bin/server"
    echo "  或使用 qemu: qemu-aarch64 -L /usr/aarch64-linux-gnu ./bin/server"
else
    echo "  sudo ./bin/server"
fi

