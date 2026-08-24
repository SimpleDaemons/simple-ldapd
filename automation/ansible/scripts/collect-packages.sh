#!/bin/bash
# Collect DEB / RPM / FreeBSD pkg / macOS dmg+pkg / source archives from the
# four build-vms hosts into dist/centralized/vVERSION/ for a GitHub release.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ANSIBLE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJECT_ROOT="$(cd "$ANSIBLE_DIR/../.." && pwd)"
INVENTORY_FILE="$ANSIBLE_DIR/inventory.ini"
PLAYBOOK="$ANSIBLE_DIR/playbook-collect.yml"
LIMIT_HOSTS=""

VERSION=$(grep -oE 'VERSION [0-9]+\.[0-9]+\.[0-9]+' "$PROJECT_ROOT/CMakeLists.txt" | head -1 | cut -d' ' -f2)
VERSION="${VERSION:-0.15.0}"
RELEASE_DIR="$PROJECT_ROOT/dist/centralized/v$VERSION"

print_info() { echo "[INFO] $1"; }
print_ok() { echo "[OK] $1"; }
print_err() { echo "[ERROR] $1" >&2; }

show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Fetch built packages from inventory group build-vms (BUILD_DEB, BUILD_RPM,
BUILD_PKG, BUILD_MACOS) into:

  dist/centralized/v$VERSION/

Also copies into dist/{linux,freebsd,macos,source}/ and writes SHA256SUMS.

OPTIONS:
    -h, --help            Show this help
    -i, --inventory FILE  Inventory (default: inventory.ini)
    -l, --limit HOSTS     Limit to specific hosts (comma-separated)
    --list                List inventory hosts and exit

EXAMPLES:
    $0
    $0 -l BUILD_DEB,BUILD_PKG
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help) show_usage; exit 0 ;;
        -i|--inventory) INVENTORY_FILE="$2"; shift 2 ;;
        -l|--limit) LIMIT_HOSTS="$2"; shift 2 ;;
        --list)
            ansible-inventory -i "$INVENTORY_FILE" --list-hosts
            exit 0
            ;;
        *) print_err "Unknown option: $1"; show_usage; exit 1 ;;
    esac
done

if ! command -v ansible-playbook >/dev/null 2>&1; then
    print_err "ansible-playbook is not installed"
    exit 1
fi
if [ ! -f "$INVENTORY_FILE" ]; then
    print_err "Inventory not found: $INVENTORY_FILE"
    exit 1
fi
if [ ! -f "$PLAYBOOK" ]; then
    print_err "Playbook not found: $PLAYBOOK"
    exit 1
fi

mkdir -p "$RELEASE_DIR" \
    "$PROJECT_ROOT/dist/linux/deb" \
    "$PROJECT_ROOT/dist/linux/rpm" \
    "$PROJECT_ROOT/dist/freebsd" \
    "$PROJECT_ROOT/dist/macos/dmg" \
    "$PROJECT_ROOT/dist/macos/pkg" \
    "$PROJECT_ROOT/dist/source"

print_info "Version: $VERSION"
print_info "Release dir: $RELEASE_DIR"
print_info "Inventory: $INVENTORY_FILE"

ANSIBLE_CMD=(ansible-playbook -i "$INVENTORY_FILE" "$PLAYBOOK"
    -e "local_release_dir=$RELEASE_DIR")
if [ -n "$LIMIT_HOSTS" ]; then
    ANSIBLE_CMD+=(--limit "$LIMIT_HOSTS")
fi

print_info "Fetching from build-vms..."
"${ANSIBLE_CMD[@]}"

copy_if_present() {
    local src="$1"
    local dest_dir="$2"
    [ -f "$src" ] || return 0
    mkdir -p "$dest_dir"
    cp -a "$src" "$dest_dir/"
}

shopt -s nullglob
for f in "$RELEASE_DIR"/simple-ldapd-*; do
    [ -f "$f" ] || continue
    base=$(basename "$f")
    case "$base" in
        *-src.tar.gz|*-src.zip) copy_if_present "$f" "$PROJECT_ROOT/dist/source" ;;
        *.deb) copy_if_present "$f" "$PROJECT_ROOT/dist/linux/deb" ;;
        *.rpm) copy_if_present "$f" "$PROJECT_ROOT/dist/linux/rpm" ;;
        *freebsd*.pkg) copy_if_present "$f" "$PROJECT_ROOT/dist/freebsd" ;;
        *.dmg) copy_if_present "$f" "$PROJECT_ROOT/dist/macos/dmg" ;;
        *.pkg) copy_if_present "$f" "$PROJECT_ROOT/dist/macos/pkg" ;;
    esac
done
shopt -u nullglob

SUMS="$RELEASE_DIR/SHA256SUMS"
: > "$SUMS"
count=0
for f in "$RELEASE_DIR"/simple-ldapd-*; do
    [ -f "$f" ] || continue
    count=$((count + 1))
    if command -v sha256sum >/dev/null 2>&1; then
        (cd "$RELEASE_DIR" && sha256sum "$(basename "$f")") >> "$SUMS"
    else
        (cd "$RELEASE_DIR" && shasum -a 256 "$(basename "$f")") >> "$SUMS"
    fi
done

print_ok "Collected $count package(s) into $RELEASE_DIR"
if [ "$count" -eq 0 ]; then
    print_err "No packages found. Run make package-all on each builder first."
    exit 1
fi

echo ""
echo "Release artifacts (attach these to the GitHub release):"
echo "  $RELEASE_DIR"
ls -la "$RELEASE_DIR"
echo ""
echo "Checksums:"
cat "$SUMS"
