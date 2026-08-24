#!/bin/sh
# Pre-uninstall for simple-ldapd RPM

set -e

PROJECT_NAME="simple-ldapd"

if [ "$1" = "0" ]; then
    if command -v systemctl >/dev/null 2>&1; then
        systemctl stop "$PROJECT_NAME" >/dev/null 2>&1 || true
        systemctl disable "$PROJECT_NAME" >/dev/null 2>&1 || true
    fi
fi

exit 0
