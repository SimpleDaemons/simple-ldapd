#!/bin/sh
# Pre-install for simple-ldapd RPM (CPack). Create the service user before files land.

set -e

PROJECT_NAME="simple-ldapd"
SERVICE_USER="simple-ldapd"
DATA_DIR="/var/lib/$PROJECT_NAME"

nologin_shell() {
    if [ -x /sbin/nologin ]; then
        echo /sbin/nologin
    elif [ -x /usr/sbin/nologin ]; then
        echo /usr/sbin/nologin
    else
        echo /bin/false
    fi
}

if ! getent group "$SERVICE_USER" >/dev/null 2>&1; then
    groupadd -r "$SERVICE_USER"
fi
if ! getent passwd "$SERVICE_USER" >/dev/null 2>&1; then
    useradd -r -M -d "$DATA_DIR" -s "$(nologin_shell)" -g "$SERVICE_USER" \
        -c "$PROJECT_NAME service user" "$SERVICE_USER"
fi

exit 0
