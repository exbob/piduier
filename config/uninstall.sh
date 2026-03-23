#!/bin/sh

set -eu

usage() {
    cat <<'EOF'
Usage:
  sudo ./uninstall.sh [--prefix PATH] [--purge-logs]

Options:
  --prefix PATH   Install prefix (default: /usr/local)
  --purge-logs    Remove /var/log/piduier directory
  -h, --help      Show this help

Environment:
  PREFIX          Same as --prefix
EOF
}

PREFIX="${PREFIX:-/usr/local}"
PURGE_LOGS=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix)
            if [ "$#" -lt 2 ]; then
                echo "Error: --prefix requires a value" >&2
                exit 1
            fi
            PREFIX="$2"
            shift 2
            ;;
        --purge-logs)
            PURGE_LOGS=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Error: unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [ "$(id -u)" -ne 0 ]; then
    echo "Error: uninstall.sh must be run as root (use sudo)." >&2
    exit 1
fi

BIN_FILE="${PREFIX}/bin/piduier"
ETC_DIR="${PREFIX}/etc/piduier"
SHARE_DIR="${PREFIX}/share/piduier"
SERVICE_DST="/etc/systemd/system/piduier.service"
LOG_DIR="/var/log/piduier"

echo "Uninstall prefix: ${PREFIX}"

if systemctl list-unit-files | grep -q '^piduier\.service'; then
    echo "Stop and disable service..."
    systemctl stop piduier 2>/dev/null || true
    systemctl disable piduier 2>/dev/null || true
fi

if [ -f "${SERVICE_DST}" ]; then
    echo "Remove service file..."
    rm -f "${SERVICE_DST}"
fi

echo "Reload systemd daemon..."
systemctl daemon-reload

echo "Remove installed files..."
rm -f "${BIN_FILE}"
rm -rf "${ETC_DIR}" "${SHARE_DIR}"

if [ "${PURGE_LOGS}" -eq 1 ]; then
    echo "Purge logs..."
    rm -rf "${LOG_DIR}"
else
    echo "Keep logs at ${LOG_DIR} (use --purge-logs to remove)."
fi

echo ""
echo "Uninstall complete."
