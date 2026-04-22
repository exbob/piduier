#!/usr/bin/env bash

set -euo pipefail

if ! command -v rsync >/dev/null 2>&1; then
	echo "Error: rsync is not installed."
	echo "Example: sudo apt-get install -y rsync"
	exit 1
fi

if ! command -v sshpass >/dev/null 2>&1; then
	echo "Error: sshpass is not installed."
	echo "Example: sudo apt-get install -y sshpass"
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="${SCRIPT_DIR}/deploy"
TARGET_IP="172.16.1.101"
USERNAME="lsc"
TARGET_PATH="~/Desktop/deploy"

if [[ ! -d "${DEPLOY_DIR}" ]]; then
	echo "Error: Local directory does not exist: ${DEPLOY_DIR}"
	exit 1
fi

read -r -s -p "Password for ${USERNAME}@${TARGET_IP}: " PASSWORD
echo
if [[ -z "${PASSWORD}" ]]; then
	echo "Error: Password cannot be empty."
	exit 1
fi

echo "Syncing ${DEPLOY_DIR} -> ${USERNAME}@${TARGET_IP}:${TARGET_PATH}"

sshpass -p "${PASSWORD}" rsync -avz --delete --progress \
	-e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
	"${DEPLOY_DIR}/" "${USERNAME}@${TARGET_IP}:${TARGET_PATH}/"

echo "Sync completed."
