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

echo "Deploy target: ${TARGET}:${TARGET_DIR}"
echo "Artifacts: ${ARTIFACT_DIR}/piduier, ${ARTIFACT_DIR}/web, ${ARTIFACT_DIR}/zlog.conf"
echo ""
echo "Step 1/3: Create target directory"
ssh "${TARGET}" "mkdir -p ${TARGET_DIR}"

echo "Step 2/3: Upload artifacts"
rsync -avz "${ARTIFACT_DIR}/piduier" "${ARTIFACT_DIR}/web" "${ARTIFACT_DIR}/zlog.conf" "${TARGET}:${TARGET_DIR}/"

echo "Step 3/3: Print run command"
echo "Deployment complete."
echo "Run on target:"
echo "  ssh ${TARGET}"
echo "  cd ${TARGET_DIR}"
echo "  ./piduier"
echo ""
echo "Note: If SSH key is not configured, your terminal will prompt for password interactively."
