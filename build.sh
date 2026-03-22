#!/bin/sh

# Build script (compile only, no deployment)
# Usage:
#   ./build.sh              # Release -> ./deploy/
#   ./build.sh debug        # Debug -> ./deploy/
#   ./build.sh clean
# Select architecture with ARCH env var: ARCH=x86 or ARCH=arm64 (default x86)

BUILD_DIR="./build"
INSTALL_PREFIX="./deploy"

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
                rm -vrf ${BUILD_DIR} deploy
                ;;
        debug)
                echo "配置 CMake (Debug)..."
                rm -vrf ${BUILD_DIR}
                cmake -S . -B ${BUILD_DIR} -D CMAKE_C_COMPILER=${CC} \
                    -D CMAKE_BUILD_TYPE=Debug
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

                echo "安装到 ${INSTALL_PREFIX}/ ..."
                rm -f "${INSTALL_PREFIX}/zlog.conf"
                cmake --install ${BUILD_DIR} --prefix ${INSTALL_PREFIX}
                if [ $? -ne 0 ]; then
                    echo "Install failed."
                    exit 1
                fi

                echo ""
                echo "Debug 编译完成！"
                echo "Note: This script only builds artifacts."
                echo "Deployment must be run as a separate step."
                echo "CMake 生成文件位置: ${BUILD_DIR}/"
                echo "可执行文件位置: ${INSTALL_PREFIX}/piduier"
                echo "Web 文件位置: ${INSTALL_PREFIX}/web/"
                echo "zlog 配置: ${INSTALL_PREFIX}/zlog.conf (stdout + DEBUG)"
                echo ""
                echo "运行方式（需在含 zlog.conf 的目录启动，或使用 cwd 下的 ./zlog.conf）:"
                echo "  cd ${INSTALL_PREFIX} && ./piduier"
                ;;
        *)
                echo "配置 CMake (Release)..."
                rm -vrf ${BUILD_DIR}
                cmake -S . -B ${BUILD_DIR} -D CMAKE_C_COMPILER=${CC} \
                    -D CMAKE_BUILD_TYPE=Release
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

                echo "安装到 ${INSTALL_PREFIX}/ ..."
                rm -f "${INSTALL_PREFIX}/zlog.conf"
                cmake --install ${BUILD_DIR} --prefix ${INSTALL_PREFIX}
                if [ $? -ne 0 ]; then
                    echo "Install failed."
                    exit 1
                fi

                echo ""
                echo "编译完成！"
                echo "Note: This script only builds artifacts."
                echo "Deployment must be run as a separate step."
                echo "CMake 生成文件位置: ${BUILD_DIR}/"
                echo "可执行文件位置: ${INSTALL_PREFIX}/piduier"
                echo "Web 文件位置: ${INSTALL_PREFIX}/web/"
                echo "zlog 配置: ${INSTALL_PREFIX}/zlog.conf (ERROR/INFO -> ./piduier.log in cwd)"
                echo ""
                echo "运行方式:"
                case ${ARCH} in
                        ARM64|arm64|aarch64)
                                echo "  注意: ARM64 版本需要在 ARM64 设备上运行，或使用 qemu 模拟"
                                echo "  cd ${INSTALL_PREFIX} && ./piduier"
                                echo "  或使用 root 权限: cd ${INSTALL_PREFIX} && sudo ./piduier"
                                echo "  或使用 qemu: qemu-aarch64 -L /usr/aarch64-linux-gnu ${INSTALL_PREFIX}/piduier"
                                ;;
                        *)
                                echo "  cd ${INSTALL_PREFIX} && ./piduier"
                                echo "  或使用 root 权限: cd ${INSTALL_PREFIX} && sudo ./piduier"
                                ;;
                esac
                ;;
esac
