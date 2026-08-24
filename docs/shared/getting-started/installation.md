# Installation

simple-ldapd is a C++17 daemon. Install from source for labs; use CPack or the scripts under `scripts/` when you need packages.

## From source

Prerequisites: CMake 3.16+, a C++17 compiler, OpenSSL, SQLite3. Details: [build guide](../../development/BUILD_GUIDE.md).

```bash
git clone https://github.com/SimpleDaemons/simple-ldapd.git
cd simple-ldapd
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Binaries are in `build/`. Default Linux `cmake --install` prefix is `/usr` (`/usr/bin/simple-ldapd`, `/etc/simple-ldapd/`). That install also creates `/var/lib/simple-ldapd` and `/var/log/simple-ldapd`.

## OpenLDAP tool names

Default client names match OpenLDAP (`ldapsearch`, `ldapadd`, …). If both must share `PATH`, pick one:

```bash
# Prefix the tools (simple-ldapsearch, …). The daemon stays simple-ldapd.
cmake -B build -DLDAP_CLI_PREFIX=simple-

# Or a private prefix so unprefixed names stay off the system PATH.
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/simple-ldapd
```

Then `cmake --install build` (or `DESTDIR` / CPack). See [build guide](../../development/BUILD_GUIDE.md) for the full option list.

## Make

```bash
make build    # gmake on FreeBSD
make test
```

## Packages

```bash
make package
```

Templates for DEB, RPM, macOS pkg/dmg, and Windows MSI/NSIS live in [packaging/](../../../packaging/README.md). Platform helpers: `scripts/build-linux.sh`, `scripts/build-macos.sh`, `scripts/build-windows.bat`.

Linux packages:

- Install the systemd unit with `ExecStart=/usr/bin/simple-ldapd --config /etc/simple-ldapd/simple-ldapd.conf --foreground`
- Install templates and examples under `/etc/simple-ldapd/` (`templates/`, `examples/`, `schemas/`) and docs under `/usr/share/doc/simple-ldapd/`
- Create the `simple-ldapd` user and `/var/lib/simple-ldapd`, `/var/log/simple-ldapd`, `/etc/simple-ldapd/tls`
- Copy `templates/production.conf` to `/etc/simple-ldapd/simple-ldapd.conf` when that file is missing
- Reload systemd; they do **not** enable or start the service (TLS and `root_password` are still unset)

## After install

1. Copy a template from `/etc/simple-ldapd/templates/` to `/etc/simple-ldapd/simple-ldapd.conf` if needed.
2. Point `schema_dir` at `/etc/simple-ldapd/schemas` (packages already install schemas there).
3. Set `root_password` and TLS files before exposing 389/636.
4. Enable a supervisor from [deployment/](../../../deployment/README.md).

`--daemon` does not fork. Run under systemd, launchd, a Windows service, or a container.

## Docker

See [deployment/examples/docker/README.md](../../../deployment/examples/docker/README.md). Confirm the image and compose files match your checkout; the daemon itself reads a config file, not `SIMPLE_LDAPD_*` environment variables.
