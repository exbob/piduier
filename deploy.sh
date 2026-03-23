#!/bin/sh

# Deployment script (deploy only, no build)
# Usage:
#   ./deploy.sh
#   TARGET=lsc@172.16.1.101 ./deploy.sh
#   TARGET=lsc@172.16.1.101 TARGET_DIR=~/Desktop/piduier ./deploy.sh

set -e

TARGET="${TARGET:-lsc@172.16.1.101}"
TARGET_DIR="${TARGET_DIR:-~/Desktop/piduier}"
ARTIFACT_DIR="./deploy"

if [ ! -x "${ARTIFACT_DIR}/piduier" ]; then
    echo "Error: ${ARTIFACT_DIR}/piduier not found."
    echo "Run build first: ARCH=arm64 ./build.sh"
    exit 1
fi

if [ ! -d "${ARTIFACT_DIR}/web" ]; then
    echo "Error: ${ARTIFACT_DIR}/web not found."
    echo "Run build first: ARCH=arm64 ./build.sh"
    exit 1
fi

echo "PiDuier: Raspberry Pi 5 40-pin interface debugging platform"
echo "Deploy target: ${TARGET}:${TARGET_DIR}"
echo "Artifacts: ${ARTIFACT_DIR}/piduier, ${ARTIFACT_DIR}/web, ${ARTIFACT_DIR}/piduier.conf, ${ARTIFACT_DIR}/install.sh, ${ARTIFACT_DIR}/uninstall.sh, ${ARTIFACT_DIR}/piduier.service"
echo ""

if [ ! -x "${ARTIFACT_DIR}/install.sh" ]; then
    echo "Error: ${ARTIFACT_DIR}/install.sh not found or not executable."
    echo "Run build first: ARCH=arm64 ./build.sh"
    exit 1
fi

if [ ! -x "${ARTIFACT_DIR}/uninstall.sh" ]; then
    echo "Error: ${ARTIFACT_DIR}/uninstall.sh not found or not executable."
    echo "Run build first: ARCH=arm64 ./build.sh"
    exit 1
fi

if [ ! -f "${ARTIFACT_DIR}/piduier.service" ]; then
    echo "Error: ${ARTIFACT_DIR}/piduier.service not found."
    echo "Run build first: ARCH=arm64 ./build.sh"
    exit 1
fi
echo "Step 1/3: Create target directory"
ssh "${TARGET}" "mkdir -p ${TARGET_DIR}"

echo "Step 2/3: Upload artifacts"
rsync -avz "${ARTIFACT_DIR}/piduier" "${ARTIFACT_DIR}/web" "${ARTIFACT_DIR}/piduier.conf" "${ARTIFACT_DIR}/install.sh" "${ARTIFACT_DIR}/uninstall.sh" "${ARTIFACT_DIR}/piduier.service" "${TARGET}:${TARGET_DIR}/"

echo "Step 3/3: Print install command"
echo "Deployment complete."
echo "Run on target:"
echo "  ssh ${TARGET}"
echo "  cd ${TARGET_DIR}"
echo "  sudo ./install.sh"
echo "  sudo systemctl start piduier"
echo "  sudo systemctl stop piduier"
echo "  sudo systemctl status piduier"
echo "  sudo ./uninstall.sh          # uninstall (keep logs)"
echo "  sudo ./uninstall.sh --purge-logs"
echo ""
echo "Note: If SSH key is not configured, your terminal will prompt for password interactively."
