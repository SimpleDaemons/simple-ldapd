#!/bin/bash
# Build simple-ldapd on remote hosts with Ansible (playbook-build.yml).

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ANSIBLE_DIR="$(dirname "$SCRIPT_DIR")"

INVENTORY_FILE="${ANSIBLE_DIR}/inventory.ini"
PLAYBOOK="${ANSIBLE_DIR}/playbook-build.yml"
GIT_BRANCH="main"
BUILD_TYPE="Release"
CLEAN_BUILD=false
RUN_TESTS=true
CREATE_PACKAGES=false
LDAP_CLI_PREFIX=""
LIMIT_HOSTS=""
ASK_BECOME_PASS=false

print_status() { echo "[INFO] $1"; }
print_success() { echo "[SUCCESS] $1"; }
print_error() { echo "[ERROR] $1"; }

show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build simple-ldapd on inventory hosts (CMake + SQLite, optional CPack).

OPTIONS:
    -h, --help              Show this help message
    -i, --inventory FILE    Inventory (default: inventory.ini)
    -b, --branch BRANCH     Git branch (default: main)
    -t, --type TYPE         Release or Debug (default: Release)
    -c, --clean             Clean the remote build directory
    -l, --limit HOSTS       Limit to specific hosts
    --cli-prefix PREFIX     CMake LDAP_CLI_PREFIX (e.g. simple-)
    --no-tests              Skip ctest
    --packages              Run make package-deb / package-rpm / package
    --ask-become-pass       Prompt for sudo password
    --list-hosts            List inventory hosts and exit

EXAMPLES:
    $0
    $0 -l ubuntu_dev --packages
    $0 --cli-prefix simple- -c
EOF
}

check_prerequisites() {
    if ! command -v ansible-playbook >/dev/null 2>&1; then
        print_error "ansible-playbook is not installed"
        exit 1
    fi
    if [ ! -f "$INVENTORY_FILE" ]; then
        print_error "Inventory not found: $INVENTORY_FILE"
        exit 1
    fi
    if [ ! -f "$PLAYBOOK" ]; then
        print_error "Playbook not found: $PLAYBOOK"
        exit 1
    fi
}

list_hosts() {
    ansible-inventory -i "$INVENTORY_FILE" --list-hosts
}

run_build() {
    ANSIBLE_CMD=(ansible-playbook -i "$INVENTORY_FILE" "$PLAYBOOK"
        -e "git_branch=$GIT_BRANCH"
        -e "build_type=$BUILD_TYPE"
        -e "clean_build=$CLEAN_BUILD"
        -e "run_tests=$RUN_TESTS"
        -e "create_packages=$CREATE_PACKAGES"
        -e "ldap_cli_prefix=$LDAP_CLI_PREFIX")
    if [ -n "$LIMIT_HOSTS" ]; then
        ANSIBLE_CMD+=(--limit "$LIMIT_HOSTS")
    fi
    if [ "$ASK_BECOME_PASS" = true ]; then
        ANSIBLE_CMD+=(--ask-become-pass)
    fi
    print_status "Running: ${ANSIBLE_CMD[*]}"
    "${ANSIBLE_CMD[@]}"
}

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help) show_usage; exit 0 ;;
        -i|--inventory) INVENTORY_FILE="$2"; shift 2 ;;
        -b|--branch) GIT_BRANCH="$2"; shift 2 ;;
        -t|--type) BUILD_TYPE="$2"; shift 2 ;;
        -c|--clean) CLEAN_BUILD=true; shift ;;
        -l|--limit) LIMIT_HOSTS="$2"; shift 2 ;;
        --cli-prefix) LDAP_CLI_PREFIX="$2"; shift 2 ;;
        --no-tests) RUN_TESTS=false; shift ;;
        --packages) CREATE_PACKAGES=true; shift ;;
        --ask-become-pass) ASK_BECOME_PASS=true; shift ;;
        --list-hosts) check_prerequisites; list_hosts; exit 0 ;;
        *) print_error "Unknown option: $1"; show_usage; exit 1 ;;
    esac
done

print_status "=== Remote build for simple-ldapd ==="
check_prerequisites
run_build
print_success "Build completed"
