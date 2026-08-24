#!/bin/bash
# Rebuild the macOS product PKG after CPack staging.
#
# CPack productbuild runs pkgbuild --root on the staging directory after
# creating Contents/Packages inside it, which incorrectly bundles Contents/
# into the payload. Installer.app then rejects the package ("incompatible
# with this version of macOS") or writes to /Contents on the system volume.
# This script rebuilds from usr/, etc/, and Library/ only.

set -euo pipefail

PROJECT_NAME="${1:-simple-ldapd}"
VERSION="${2:-0.15.0}"
BUILD_DIR="${3:-build}"
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
IDENTIFIER="com.simpledaemons.${PROJECT_NAME}"
POSTINSTALL="$ROOT/packaging/macos/pkg/scripts/postinstall"

shopt -s nullglob
stages=("$BUILD_DIR/_CPack_Packages/Darwin/productbuild/${PROJECT_NAME}-"*)
shopt -u nullglob

if [ ${#stages[@]} -eq 0 ]; then
    echo "error: no CPack productbuild staging directory under $BUILD_DIR" >&2
    exit 1
fi

STAGE="${stages[0]}"
PKG_BASENAME="$(basename "$STAGE")"
OUT_PKG="$BUILD_DIR/${PKG_BASENAME}.pkg"
PAYLOAD="$(mktemp -d)"
SCRIPTS="$(mktemp -d)"

cleanup() {
    rm -rf "$PAYLOAD" "$SCRIPTS"
}
trap cleanup EXIT

if [ ! -d "$STAGE/usr" ] && [ ! -d "$STAGE/etc" ] && [ ! -d "$STAGE/Library" ]; then
    echo "error: staging directory missing usr/, etc/, or Library/: $STAGE" >&2
    exit 1
fi

mkdir -p "$PAYLOAD"
[ -d "$STAGE/usr" ] && cp -R "$STAGE/usr" "$PAYLOAD/"
[ -d "$STAGE/etc" ] && cp -R "$STAGE/etc" "$PAYLOAD/"
[ -d "$STAGE/Library" ] && cp -R "$STAGE/Library" "$PAYLOAD/"
[ -d "$STAGE/var" ] && cp -R "$STAGE/var" "$PAYLOAD/"

if [ ! -f "$POSTINSTALL" ]; then
    echo "error: postinstall not found: $POSTINSTALL" >&2
    exit 1
fi
cp "$POSTINSTALL" "$SCRIPTS/postinstall"
chmod 0755 "$SCRIPTS/postinstall"

DIST_FILE=""
for f in "$STAGE/Contents/distribution.dist" "$STAGE/Contents/Distribution" \
         "$STAGE/distribution.dist"; do
    if [ -f "$f" ]; then
        DIST_FILE="$f"
        break
    fi
done
if [ -z "$DIST_FILE" ]; then
    echo "error: CPack distribution XML not found under $STAGE" >&2
    exit 1
fi

# Installer.app treats <product> as OS-update metadata and rejects the pkg.
# Allow both Intel and Apple silicon (Rosetta runs an x86_64 slice).
sed -i '' '/<product /d' "$DIST_FILE"
if grep -q 'hostArchitectures=' "$DIST_FILE"; then
    :
else
    sed -i '' 's/<options /<options hostArchitectures="arm64,x86_64" /' "$DIST_FILE"
fi

mkdir -p "$STAGE/Contents/Packages"
PKGBUILD_ARGS=(
    --root "$PAYLOAD"
    --install-location /
    --identifier "$IDENTIFIER"
    --version "$VERSION"
    --scripts "$SCRIPTS"
)
if pkgbuild --help 2>&1 | grep -q -- '--min-os-version'; then
    PKGBUILD_ARGS+=(--min-os-version 12.0)
fi
pkgbuild "${PKGBUILD_ARGS[@]}" "$STAGE/Contents/Packages/${PROJECT_NAME}"

RESOURCES="$STAGE/Contents"
[ -d "$STAGE/Contents/Resources" ] && RESOURCES="$STAGE/Contents/Resources"

productbuild --distribution "$DIST_FILE" \
    --package-path "$STAGE/Contents/Packages" \
    --resources "$RESOURCES" \
    --version "$VERSION" \
    "$OUT_PKG"

echo "Rebuilt macOS PKG: $OUT_PKG"
