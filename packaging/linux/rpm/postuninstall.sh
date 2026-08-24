#!/bin/bash
# Post-uninstallation script for simple-ldapd RPM

set -e

# Reload systemd
systemctl daemon-reload

exit 0

