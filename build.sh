#!/bin/sh

# Build script (compile only, no deployment)
# Usage: ./build.sh [clean]
# Select architecture with ARCH env var: ARCH=x86 or ARCH=arm64 (default x86)

BUILD_DIR="./build"
RELEASE_DIR="."

# 设置架构，默认为 x86
ARCH=${ARCH:-x86}

# 根据 ARCH 设置编译器
case ${ARCH} in
        x86|X86)
                CC="gcc"
                echo "Building for x86 (using gcc)"
                ;;
        ARM64|arm64|aarch64)
                CC="aarch64-linux-gnu-gcc"
                echo "Building for ARM64 (using aarch64-linux-gnu-gcc)"
                ;;
        *)
                echo "Error: Unknown ARCH=${ARCH}. Supported: x86, ARM64"
                exit 1
                ;;
esac

# 检查编译器是否存在
if ! command -v ${CC} > /dev/null 2>&1; then
    echo "Error: Compiler ${CC} not found"
    echo "Please install the corresponding toolchain"
    exit 1
fi

case ${1} in
        clean)
                echo "Clean..."
                rm -vrf ${BUILD_DIR} bin
                ;;
        *)
                echo "配置 CMake..."
                rm -vrf ${BUILD_DIR}
                cmake -S . -B ${BUILD_DIR} -D CMAKE_C_COMPILER=${CC}
                if [ $? -ne 0 ]; then
                    echo ""
                    echo "CMake configure failed."
                    exit 1
                fi
                
                echo "开始编译..."
                cmake --build ${BUILD_DIR} -j$(nproc)
                if [ $? -ne 0 ]; then
                    echo "Build failed."
                    exit 1
                fi
                
                echo "安装可执行文件到 bin/ 目录..."
                cmake --install ${BUILD_DIR} --prefix ${RELEASE_DIR}
                if [ $? -ne 0 ]; then
                    echo "Install failed."
                    exit 1
                fi
                
                echo ""
                echo "编译完成！"
                echo "Note: This script only builds artifacts."
                echo "Deployment must be run as a separate step."
                echo "CMake 生成文件位置: ${BUILD_DIR}/"
                echo "可执行文件位置: ./bin/piduier"
                echo "Web 文件位置: ./bin/web/"
                echo ""
                echo "运行方式:"
                case ${ARCH} in
                        ARM64|arm64|aarch64)
                                echo "  注意: ARM64 版本需要在 ARM64 设备上运行，或使用 qemu 模拟"
                                echo "  ./bin/piduier"
                                echo "  或使用 root 权限: sudo ./bin/piduier"
                                echo "  或使用 qemu: qemu-aarch64 -L /usr/aarch64-linux-gnu ./bin/piduier"
                                ;;
                        *)
                                echo "  ./bin/piduier"
                                echo "  或使用 root 权限: sudo ./bin/piduier"
                                ;;
                esac
                ;;
esac

