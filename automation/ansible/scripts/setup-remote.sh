#!/bin/bash
# Remote setup script for Ansible deployment
# Usage: ./setup-remote.sh

set -e

PROJECT_NAME="simple-ldapd"
PROJECT_USER="simple-ldapd"
PROJECT_DIR="/opt/$PROJECT_NAME"

echo "Setting up $PROJECT_NAME on remote host..."

# Create build user if it doesn't exist
if ! id "$PROJECT_USER" &>/dev/null; then
    echo "Creating user $PROJECT_USER..."
    sudo useradd -m -s /bin/bash "$PROJECT_USER"
fi

# Create project directory
sudo mkdir -p "$PROJECT_DIR"
sudo chown "$PROJECT_USER:$PROJECT_USER" "$PROJECT_DIR"

# Create build directory
sudo mkdir -p "$PROJECT_DIR/build"
sudo chown "$PROJECT_USER:$PROJECT_USER" "$PROJECT_DIR/build"

# Create log directory
sudo mkdir -p "/var/log/$PROJECT_NAME"
sudo chown "$PROJECT_USER:$PROJECT_USER" "/var/log/$PROJECT_NAME"

# Create config directory
sudo mkdir -p "/etc/$PROJECT_NAME"
sudo chown "$PROJECT_USER:$PROJECT_USER" "/etc/$PROJECT_NAME"

echo "Setup complete!"

