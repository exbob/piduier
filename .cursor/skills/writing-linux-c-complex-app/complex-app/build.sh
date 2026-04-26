#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
DEPLOY_DIR="${ROOT_DIR}/deploy"

ARCH="${ARCH:-x86}"
TOOLCHAIN_FILE=""

MODE="${1:-release}"
BUILD_TYPE="Release"

help()
{
	echo "Usage: [ARCH=aarch64|arm64|x86_64|x86] ./build.sh [MODE]"
	echo "MODE: release|debug"
	echo "Example: ./build.sh clean"
	echo "Example: ./build.sh"
	echo "Example: ./build.sh debug"
	echo "Example: ARCH=arm64 ./build.sh"
}

case "${ARCH}" in
	aarch64|arm64|ARM64)
		TOOLCHAIN_FILE="${ROOT_DIR}/cmake/toolchain-aarch64.cmake"
		;;
	x86_64|x86|X86)
		TOOLCHAIN_FILE="${ROOT_DIR}/cmake/toolchain-x86_64.cmake"
		;;
	*)
		echo "Unsupported ARCH: ${ARCH}"
		echo "Supported values: arm64|aarch64|ARM64, x86_64|x86|X86"
		exit 1
		;;
esac

case "${MODE}" in
	help|-h|--help)
		help
		exit 0
		;;
	clean)
		rm -rf "${BUILD_DIR}" "${DEPLOY_DIR}"
		mkdir -p "${BUILD_DIR}" "${DEPLOY_DIR}"
		echo "Clean done: ${BUILD_DIR} and ${DEPLOY_DIR}"
		exit 0
		;;
	debug)
		BUILD_TYPE="Debug"
		;;
	*)
		BUILD_TYPE="Release"
		;;
esac

rm -rf "${BUILD_DIR}" "${DEPLOY_DIR}"
mkdir -p "${BUILD_DIR}" "${DEPLOY_DIR}"

CMAKE_BUILD_ARGS=(
	-S "${ROOT_DIR}"
	-B "${BUILD_DIR}"
	-D CMAKE_BUILD_TYPE="${BUILD_TYPE}"
	-D CMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}"
)

cmake "${CMAKE_BUILD_ARGS[@]}"
cmake --build "${BUILD_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${DEPLOY_DIR}"

echo "Build done (${BUILD_TYPE}), artifacts in ${DEPLOY_DIR}"
