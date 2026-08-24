#!/bin/bash
# Build script for remote deployment
# Usage: ./build.sh [clean|test|install]

set -e

PROJECT_DIR="/opt/simple-ldapd"
BUILD_USER="simple-ldapd"

case "${1:-build}" in
    "clean")
        echo "Cleaning build directory..."
        sudo -u "$BUILD_USER" rm -rf "$PROJECT_DIR/build"
        sudo -u "$BUILD_USER" mkdir -p "$PROJECT_DIR/build"
        ;;
    "test")
        echo "Running tests..."
        cd "$PROJECT_DIR/build"
        sudo -u "$BUILD_USER" make test
        ;;
    "install")
        echo "Installing service..."
        sudo systemctl enable simple-ldapd
        sudo systemctl start simple-ldapd
        ;;
    "build")
        echo "Building project..."
        cd "$PROJECT_DIR/build"
        sudo -u "$BUILD_USER" cmake ..
        sudo -u "$BUILD_USER" make -j$(nproc)
        ;;
    *)
        echo "Usage: $0 [clean|test|install|build]"
        exit 1
        ;;
esac

