#!/bin/bash
# Remote setup for Ansible / VM hosts. Creates the service user and FHS dirs.

set -e

PROJECT_NAME="simple-ldapd"
PROJECT_USER="simple-ldapd"
PROJECT_DIR="/opt/$PROJECT_NAME"
CONFIG_DIR="/etc/$PROJECT_NAME"
DATA_DIR="/var/lib/$PROJECT_NAME"
LOG_DIR="/var/log/$PROJECT_NAME"

NOLOGIN=/usr/sbin/nologin
[ -x "$NOLOGIN" ] || NOLOGIN=/sbin/nologin
[ -x "$NOLOGIN" ] || NOLOGIN=/bin/bash

echo "Setting up $PROJECT_NAME on $(hostname)..."

if ! getent group "$PROJECT_USER" >/dev/null 2>&1; then
    sudo groupadd --system "$PROJECT_USER" 2>/dev/null || sudo groupadd -r "$PROJECT_USER"
fi

if ! getent passwd "$PROJECT_USER" >/dev/null 2>&1; then
    echo "Creating user $PROJECT_USER..."
    sudo useradd --system --no-create-home --home-dir "$DATA_DIR" \
        --shell /bin/bash --gid "$PROJECT_USER" \
        --comment "$PROJECT_NAME service user" "$PROJECT_USER" 2>/dev/null || \
    sudo useradd -r -M -d "$DATA_DIR" -s /bin/bash -g "$PROJECT_USER" "$PROJECT_USER"
fi

sudo mkdir -p "$PROJECT_DIR/build" "$CONFIG_DIR/tls" "$DATA_DIR" "$LOG_DIR"
sudo chown "$PROJECT_USER:$PROJECT_USER" "$PROJECT_DIR" "$PROJECT_DIR/build"
sudo chown root:"$PROJECT_USER" "$CONFIG_DIR" "$CONFIG_DIR/tls"
sudo chmod 0750 "$CONFIG_DIR" "$CONFIG_DIR/tls"
sudo chown "$PROJECT_USER:$PROJECT_USER" "$DATA_DIR" "$LOG_DIR"
sudo chmod 0750 "$DATA_DIR" "$LOG_DIR"

echo "Setup complete. Build with automation/ansible/scripts/build.sh"
