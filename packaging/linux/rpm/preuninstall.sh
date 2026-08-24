#!/bin/bash
# Pre-uninstallation script for simple-ldapd RPM

set -e

PROJECT_NAME="simple-ldapd"

# Stop service before removal
if [ "$1" -eq 0 ]; then
    systemctl stop "$PROJECT_NAME" 2>/dev/null || true
    systemctl disable "$PROJECT_NAME" 2>/dev/null || true
fi

exit 0

