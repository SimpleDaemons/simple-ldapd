#!/bin/bash
# Post-installation script for simple-ldapd RPM

set -e

PROJECT_NAME="simple-ldapd"
SERVICE_USER="simple-ldapd"

# Create service user if it doesn't exist
if ! id "$SERVICE_USER" &>/dev/null; then
    useradd -r -s /sbin/nologin -d /var/lib/$PROJECT_NAME -c "$PROJECT_NAME service user" "$SERVICE_USER"
fi

# Set ownership
chown -R "$SERVICE_USER:$SERVICE_USER" /etc/$PROJECT_NAME 2>/dev/null || true
chown -R "$SERVICE_USER:$SERVICE_USER" /var/log/$PROJECT_NAME 2>/dev/null || true
chown -R "$SERVICE_USER:$SERVICE_USER" /var/lib/$PROJECT_NAME 2>/dev/null || true

# Enable and start service
systemctl daemon-reload
systemctl enable "$PROJECT_NAME" 2>/dev/null || true
systemctl start "$PROJECT_NAME" 2>/dev/null || true

exit 0

