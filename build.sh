#!/bin/sh

# Build script (compile only, no deployment)
# Usage:
#   ./build.sh              # Release -> ./deploy/
#   ./build.sh debug        # Debug -> ./deploy/
#   ./build.sh clean
# Select architecture with ARCH env var: ARCH=x86 or ARCH=arm64 (default x86)

BUILD_DIR="./build"
INSTALL_PREFIX="./deploy"

# Default architecture: arm64
ARCH=${ARCH:-arm64}

# Select compiler by ARCH
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

# Ensure compiler exists
if ! command -v ${CC} > /dev/null 2>&1; then
    echo "Error: Compiler ${CC} not found"
    echo "Install the required toolchain first."
    exit 1
fi

case ${1} in
        clean)
                echo "Cleaning..."
                rm -vrf ${BUILD_DIR} deploy
                ;;
        debug)
                echo "Configuring (Debug)..."
                rm -vrf ${BUILD_DIR}
                cmake -S . -B ${BUILD_DIR} -D CMAKE_C_COMPILER=${CC} \
                    -D CMAKE_BUILD_TYPE=Debug
                if [ $? -ne 0 ]; then
                    echo ""
                    echo "CMake configure failed."
                    exit 1
                fi

                echo "Building..."
                cmake --build ${BUILD_DIR} -j$(nproc)
                if [ $? -ne 0 ]; then
                    echo "Build failed."
                    exit 1
                fi

                echo "Installing to ${INSTALL_PREFIX}/..."
                rm -f "${INSTALL_PREFIX}/zlog.conf" "${INSTALL_PREFIX}/piduier.conf"
                cmake --install ${BUILD_DIR} --prefix ${INSTALL_PREFIX}
                if [ $? -ne 0 ]; then
                    echo "Install failed."
                    exit 1
                fi

                echo ""
                echo "Debug build complete."
                echo "Build only (no deployment)."
                echo "Build dir: ${BUILD_DIR}/"
                echo "Binary: ${INSTALL_PREFIX}/piduier"
                echo "Web dir: ${INSTALL_PREFIX}/web/"
                echo "Config: ${INSTALL_PREFIX}/piduier.conf (debug logs to stdout)"
                echo ""
                echo "Run (from config directory):"
                echo "  cd ${INSTALL_PREFIX} && ./piduier"
                ;;
        *)
                echo "Configuring (Release)..."
                rm -vrf ${BUILD_DIR}
                cmake -S . -B ${BUILD_DIR} -D CMAKE_C_COMPILER=${CC} \
                    -D CMAKE_BUILD_TYPE=Release
                if [ $? -ne 0 ]; then
                    echo ""
                    echo "CMake configure failed."
                    exit 1
                fi

                echo "Building..."
                cmake --build ${BUILD_DIR} -j$(nproc)
                if [ $? -ne 0 ]; then
                    echo "Build failed."
                    exit 1
                fi

                echo "Installing to ${INSTALL_PREFIX}/..."
                rm -f "${INSTALL_PREFIX}/zlog.conf" "${INSTALL_PREFIX}/piduier.conf"
                cmake --install ${BUILD_DIR} --prefix ${INSTALL_PREFIX}
                if [ $? -ne 0 ]; then
                    echo "Install failed."
                    exit 1
                fi

                echo ""
                echo "Release build complete."
                echo "Build only (no deployment)."
                echo "Build dir: ${BUILD_DIR}/"
                echo "Binary: ${INSTALL_PREFIX}/piduier"
                echo "Web dir: ${INSTALL_PREFIX}/web/"
                echo "Config: ${INSTALL_PREFIX}/piduier.conf (see zlog.log_file)"
                echo ""
                echo "Run:"
                case ${ARCH} in
                        ARM64|arm64|aarch64)
                                echo "  Note: ARM64 binary needs ARM64 host or qemu."
                                echo "  cd ${INSTALL_PREFIX} && ./piduier"
                                echo "  Or run as root: cd ${INSTALL_PREFIX} && sudo ./piduier"
                                echo "  Or use qemu: qemu-aarch64 -L /usr/aarch64-linux-gnu ${INSTALL_PREFIX}/piduier"
                                ;;
                        *)
                                echo "  cd ${INSTALL_PREFIX} && ./piduier"
                                echo "  Or run as root: cd ${INSTALL_PREFIX} && sudo ./piduier"
                                ;;
                esac
                ;;
esac
