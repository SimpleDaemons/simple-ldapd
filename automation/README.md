# simple-ldapd - Automation

This directory contains all automation scripts and configuration files for setting up and managing the simple-ldapd development environment.

## Directory Structure

```
automation/
├── ansible/                  # Ansible automation
│   ├── playbook.yml         # Lab setup (SQLite, FHS dirs, optional unit install)
│   ├── playbook-build.yml   # Remote CMake/CPack builds
│   ├── inventory.ini        # Ansible inventory file
│   ├── requirements.yml     # Ansible Galaxy requirements
│   ├── Makefile.vm          # Makefile for VM operations
│   ├── vagrant-boxes.yml   # Vagrant box configurations
│   ├── scripts/             # Shell scripts for VM operations
│   │   ├── vm-ssh          # SSH wrapper for VM
│   │   ├── vm-build        # Build script for VM
│   │   ├── vm-test         # Test script for VM
│   │   ├── setup-remote.sh # Remote setup script
│   │   └── build.sh        # Build script
│   └── templates/          # Configuration templates
├── ci/                      # CI/CD configuration
│   ├── Jenkinsfile         # Jenkins pipeline
│   └── .travis.yml         # Travis CI configuration
├── docker/                  # Docker configuration
│   ├── Dockerfile          # Docker image definition
│   ├── docker-compose.yml  # Docker Compose configuration
│   └── examples/           # Docker examples
└── vagrant/                 # Vagrant configuration
    ├── Vagrantfile         # Main Vagrantfile
    └── virtuals/           # Multi-VM configurations
        ├── ubuntu_dev/
        └── centos_dev/
```

## Quick Start

### Using Docker

```bash
# Build and run with Docker Compose
cd automation/docker
docker-compose up -d

# Or from project root
docker-compose -f automation/docker/docker-compose.yml up -d
```

### Using Vagrant

```bash
# Start VM
cd automation/vagrant
vagrant up

# SSH into VM
vagrant ssh

# Build project
./automation/ansible/scripts/vm-build
```

### Using Ansible

```bash
# Run playbook
ansible-playbook -i automation/ansible/inventory.ini automation/ansible/playbook.yml
```

## CI/CD

### Jenkins

The Jenkins pipeline is located at `automation/ci/Jenkinsfile`. It supports:
- Multi-platform builds (Linux, macOS, Windows)
- Automated testing
- Static analysis
- Package generation
- Docker image building

### Travis CI

The Travis CI configuration is located at `automation/ci/.travis.yml`. It provides:
- Automated builds on push
- Multi-platform testing
- Code coverage reporting

## Docker

Docker files are located in `automation/docker/`:
- `Dockerfile` - Multi-stage build for different distributions
- `docker-compose.yml` - Development and production configurations
- `examples/` - Example Docker configurations

## Vagrant

Vagrant configuration is in `automation/vagrant/`:
- `Vagrantfile` - Main Vagrant configuration
- `virtuals/` - Multi-VM configurations for different distributions

## Ansible

Ansible lives in `automation/ansible/`. It follows the same layout as packaging: SQLite, `/usr` install prefix, FHS data/log dirs, and the shipped systemd unit (`--config` then `--foreground`). Packages and `install_service` do not start the daemon.

```
automation/ansible/
├── ansible.cfg
├── playbook.yml            # Vagrant / lab setup (cmake, ctest, optional install)
├── playbook-build.yml      # Remote builders (optional CPack)
├── inventory.ini
├── requirements.yml
├── Makefile.vm
├── vagrant-boxes.yml
└── scripts/
    ├── vm-ssh
    ├── vm-build            # cmake with ENABLE_SQLITE; install copies the unit
    ├── vm-test             # ctest
    ├── setup-remote.sh     # service user + /var/lib + /var/log + tls
    ├── build.sh
    ├── remote-build.sh     # ansible-playbook playbook-build.yml
    ├── collect-packages.sh # pull artifacts from the four builders for a GitHub release
    └── organize-packages.sh
```

```bash
# Lab VM (from the project root)
ansible-playbook -i automation/ansible/inventory.ini automation/ansible/playbook.yml

# Remote build + packages
./automation/ansible/scripts/remote-build.sh --packages
./automation/ansible/scripts/remote-build.sh --cli-prefix simple-

# After make package-all on BUILD_DEB, BUILD_RPM, BUILD_PKG, BUILD_MACOS:
./automation/ansible/scripts/collect-packages.sh
# Artifacts: dist/centralized/vVERSION/ (+ SHA256SUMS)

# Optional: install the unit after a local VM build (does not start)
./automation/ansible/scripts/vm-build ubuntu_dev install
```

CMake extras: `-e ldap_cli_prefix=simple-`, `-e install_prefix=/usr`, `-e install_service=true` (copies config/unit; you still set `root_password` and TLS before `systemctl enable --now`).

---

*For detailed documentation, see the individual README files in each subdirectory.*

