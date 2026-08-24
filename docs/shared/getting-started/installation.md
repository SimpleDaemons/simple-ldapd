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

Binaries are in `build/`. Default names match OpenLDAP (`ldapsearch`, …). Install with `-DLDAP_CLI_PREFIX=simple-` (or a private `CMAKE_INSTALL_PREFIX`) if you need to keep OpenLDAP on `PATH`.

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

## After install

1. Copy a template from `config/templates/` to `/etc/simple-ldapd/simple-ldapd.conf` (or keep a local path).
2. Copy `schemas/` to the `schema_dir` you configured.
3. Set `root_password` and TLS files before exposing 389/636.
4. Enable a supervisor from [deployment/](../../../deployment/README.md).

`--daemon` does not fork. Run under systemd, launchd, a Windows service, or a container.

## Docker

See [deployment/examples/docker/README.md](../../../deployment/examples/docker/README.md). Confirm the image and compose files match your checkout; the daemon itself reads a config file, not `SIMPLE_LDAPD_*` environment variables.
