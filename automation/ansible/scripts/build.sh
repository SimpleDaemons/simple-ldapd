#!/bin/bash
# Build script for remote / VM deployment
# Usage: ./build.sh [clean|test|install|build]

set -e

PROJECT_DIR="/opt/simple-ldapd"
BUILD_USER="simple-ldapd"
CONFIG_DIR="/etc/simple-ldapd"
DATA_DIR="/var/lib/simple-ldapd"
LOG_DIR="/var/log/simple-ldapd"
CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_TESTS=ON -DENABLE_SSL=ON -DENABLE_SQLITE=ON"

nproc_jobs() {
    command -v nproc >/dev/null 2>&1 && nproc || sysctl -n hw.ncpu
}

ensure_runtime_dirs() {
    sudo mkdir -p "$CONFIG_DIR/tls" "$DATA_DIR" "$LOG_DIR"
    sudo chown root:"$BUILD_USER" "$CONFIG_DIR" "$CONFIG_DIR/tls" 2>/dev/null || true
    sudo chmod 0750 "$CONFIG_DIR" "$CONFIG_DIR/tls" 2>/dev/null || true
    sudo chown "$BUILD_USER:$BUILD_USER" "$DATA_DIR" "$LOG_DIR"
    sudo chmod 0750 "$DATA_DIR" "$LOG_DIR"
}

case "${1:-build}" in
    clean)
        echo "Cleaning build directory..."
        sudo -u "$BUILD_USER" rm -rf "$PROJECT_DIR/build"
        sudo -u "$BUILD_USER" mkdir -p "$PROJECT_DIR/build"
        ;;
    test)
        echo "Running tests..."
        cd "$PROJECT_DIR/build"
        sudo -u "$BUILD_USER" ctest --output-on-failure
        ;;
    install)
        echo "Installing binaries, unit, and runtime dirs..."
        ensure_runtime_dirs
        sudo cmake --install "$PROJECT_DIR/build"
        sudo cp "$PROJECT_DIR/deployment/systemd/simple-ldapd.service" \
            /etc/systemd/system/simple-ldapd.service
        if [ ! -f "$CONFIG_DIR/simple-ldapd.conf" ] && [ -f "$PROJECT_DIR/config/templates/production.conf" ]; then
            sudo cp "$PROJECT_DIR/config/templates/production.conf" "$CONFIG_DIR/simple-ldapd.conf"
            sudo chown root:"$BUILD_USER" "$CONFIG_DIR/simple-ldapd.conf"
            sudo chmod 0640 "$CONFIG_DIR/simple-ldapd.conf"
        fi
        sudo systemctl daemon-reload
        echo "Unit installed. Set root_password and TLS, then: sudo systemctl enable --now simple-ldapd"
        ;;
    build)
        echo "Building project..."
        mkdir -p "$PROJECT_DIR/build"
        cd "$PROJECT_DIR/build"
        sudo -u "$BUILD_USER" cmake .. $CMAKE_FLAGS
        sudo -u "$BUILD_USER" make -j"$(nproc_jobs)"
        ;;
    *)
        echo "Usage: $0 [clean|test|install|build]"
        exit 1
        ;;
esac
