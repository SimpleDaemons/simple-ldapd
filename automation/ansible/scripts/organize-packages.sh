#!/bin/bash
# Move CPack/make package outputs into dist/centralized/vVERSION/

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
VERSION=$(grep '^project.*VERSION' "$PROJECT_ROOT/CMakeLists.txt" 2>/dev/null \
    | sed -n 's/.*VERSION \([0-9.]*\).*/\1/p' | head -1)
VERSION="${VERSION:-0.15.0}"
DEST="$PROJECT_ROOT/dist/centralized/v$VERSION"
mkdir -p "$DEST"

moved=0
for dir in "$PROJECT_ROOT/build" "$PROJECT_ROOT/dist"; do
    [ -d "$dir" ] || continue
    for f in "$dir"/simple-ldapd-*.deb "$dir"/simple-ldapd-*.rpm \
             "$dir"/simple-ldapd-*.dmg "$dir"/simple-ldapd-*.pkg \
             "$dir"/simple-ldapd-*.tar.gz "$dir"/simple-ldapd-*.zip; do
        [ -f "$f" ] || continue
        cp -a "$f" "$DEST/"
        moved=$((moved + 1))
        echo "copied $(basename "$f") -> $DEST"
    done
done

echo "organized $moved package(s) into $DEST"
