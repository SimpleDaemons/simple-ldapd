#!/bin/sh
# Post-install for simple-ldapd RPM (CPack). Own data/log dirs; do not start.

set -e

PROJECT_NAME="simple-ldapd"
SERVICE_USER="simple-ldapd"
CONFIG_DIR="/etc/$PROJECT_NAME"
DATA_DIR="/var/lib/$PROJECT_NAME"
LOG_DIR="/var/log/$PROJECT_NAME"

mkdir -p "$CONFIG_DIR/tls" "$DATA_DIR" "$LOG_DIR"
chown root:"$SERVICE_USER" "$CONFIG_DIR" "$CONFIG_DIR/tls" 2>/dev/null || true
chmod 0750 "$CONFIG_DIR" "$CONFIG_DIR/tls" 2>/dev/null || true
chown "$SERVICE_USER:$SERVICE_USER" "$DATA_DIR" "$LOG_DIR" 2>/dev/null || true
chmod 0750 "$DATA_DIR" "$LOG_DIR" 2>/dev/null || true

if [ ! -f "$CONFIG_DIR/$PROJECT_NAME.conf" ] && [ -f "$CONFIG_DIR/templates/production.conf" ]; then
    cp "$CONFIG_DIR/templates/production.conf" "$CONFIG_DIR/$PROJECT_NAME.conf"
    chown root:"$SERVICE_USER" "$CONFIG_DIR/$PROJECT_NAME.conf"
    chmod 0640 "$CONFIG_DIR/$PROJECT_NAME.conf"
elif [ -f "$CONFIG_DIR/$PROJECT_NAME.conf" ]; then
    chown root:"$SERVICE_USER" "$CONFIG_DIR/$PROJECT_NAME.conf" 2>/dev/null || true
    chmod 0640 "$CONFIG_DIR/$PROJECT_NAME.conf" 2>/dev/null || true
fi

if command -v systemd-tmpfiles >/dev/null 2>&1; then
    systemd-tmpfiles --create "/usr/lib/tmpfiles.d/$PROJECT_NAME.conf" >/dev/null 2>&1 || true
fi
if command -v systemctl >/dev/null 2>&1; then
    systemctl daemon-reload >/dev/null 2>&1 || true
fi

exit 0
