#!/bin/sh

set -eu

usage() {
    cat <<'EOF'
Usage:
  sudo ./install.sh [--prefix PATH]

Options:
  --prefix PATH   Install prefix (default: /usr/local)
  -h, --help      Show this help

Environment:
  PREFIX          Same as --prefix
EOF
}

PREFIX="${PREFIX:-/usr/local}"

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
    echo "Error: install.sh must be run as root (use sudo)." >&2
    exit 1
fi

for required in piduier piduier.conf piduier.service; do
    if [ ! -e "./${required}" ]; then
        echo "Error: ./${required} not found in current directory." >&2
        exit 1
    fi
done

if [ ! -d "./web" ]; then
    echo "Error: ./web directory not found in current directory." >&2
    exit 1
fi

BIN_DIR="${PREFIX}/bin"
ETC_DIR="${PREFIX}/etc/piduier"
SHARE_DIR="${PREFIX}/share/piduier"
WEB_DIR="${SHARE_DIR}/web"
LOG_DIR="/var/log/piduier"
CONF_DST="${ETC_DIR}/piduier.conf"
SERVICE_DST="/etc/systemd/system/piduier.service"

echo "Install prefix: ${PREFIX}"
echo "Create directories..."
mkdir -p "${BIN_DIR}" "${ETC_DIR}" "${SHARE_DIR}" "${LOG_DIR}"

echo "Install binary..."
install -m 0755 "./piduier" "${BIN_DIR}/piduier"

echo "Install web assets..."
rm -rf "${WEB_DIR}"
mkdir -p "${WEB_DIR}"
cp -a "./web/." "${WEB_DIR}/"

echo "Install and patch config..."
cp "./piduier.conf" "${CONF_DST}"

escaped_web_dir=$(printf '%s\n' "${WEB_DIR}" | sed 's/[\\/&]/\\&/g')
escaped_log_file=$(printf '%s\n' "${LOG_DIR}/piduier.log" | sed 's/[\\/&]/\\&/g')

sed -i \
    -e "s|\"web_root\"[[:space:]]*:[[:space:]]*\"[^\"]*\"|\"web_root\": \"${escaped_web_dir}\"|" \
    -e "s|\"log_file\"[[:space:]]*:[[:space:]]*\"[^\"]*\"|\"log_file\": \"${escaped_log_file}\"|" \
    "${CONF_DST}"

echo "Install systemd service..."
sed "s|<prefix>|${PREFIX}|g" "./piduier.service" > "${SERVICE_DST}"
chmod 0644 "${SERVICE_DST}"

echo "Reload systemd daemon..."
systemctl daemon-reload

echo ""
echo "Install complete."
echo "Installed files:"
echo "  - ${BIN_DIR}/piduier"
echo "  - ${CONF_DST}"
echo "  - ${WEB_DIR}/"
echo "  - ${SERVICE_DST}"
echo "  - ${LOG_DIR}/piduier.log"
echo ""
echo "Next commands:"
echo "  systemctl status piduier"
echo "  systemctl start piduier"
echo "  systemctl stop piduier"
echo "  systemctl enable piduier"
