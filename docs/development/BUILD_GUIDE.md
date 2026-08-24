# Build guide

Requires CMake 3.16+, a C++17 compiler (Clang, GCC, or MSVC), OpenSSL, and SQLite3. jsoncpp is optional and unused by the current key/value config parser.

## Prerequisites

macOS (Homebrew):

```bash
brew install cmake openssl jsoncpp sqlite
```

Debian/Ubuntu:

```bash
sudo apt-get install build-essential cmake pkg-config libssl-dev libjsoncpp-dev libsqlite3-dev
```

FreeBSD:

```bash
pkg install cmake openssl jsoncpp sqlite3 gmake
gmake build
```

Windows: Visual Studio 2019+ with CMake, plus OpenSSL available to CMake.

## CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Binaries land in `build/`: `simple-ldapd`, `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`, `ldapcompare`, `ldapwhoami`. On Linux the default `CMAKE_INSTALL_PREFIX` is `/usr`. `cmake --install` also creates `/var/lib/simple-ldapd` and `/var/log/simple-ldapd`.

### Options

| Option | Default | Purpose |
|--------|---------|---------|
| `ENABLE_TESTS` | ON | Build `test_ldap_*` and register them with CTest |
| `ENABLE_SSL` | ON | Link OpenSSL; required for LDAPS and StartTLS |
| `ENABLE_JSON` | ON | Link jsoncpp if found (config is still key=value) |
| `ENABLE_SQLITE` | ON | Link SQLite3; required for `backend = sqlite` |
| `ENABLE_PACKAGING` | ON | CPack targets |
| `ENABLE_STATIC_LINKING` | OFF | Self-contained binaries |
| `LDAP_CLI_PREFIX` | (empty) | Prefix OpenLDAP-style tool names. `-DLDAP_CLI_PREFIX=simple-` installs `simple-ldapsearch` and so on; the daemon remains `simple-ldapd`. Use this (or a private `CMAKE_INSTALL_PREFIX`) if OpenLDAP clients must stay on `PATH`. |
| `BUILD_SHARED_LIBS` | OFF | Shared library (static `simple-ldapd_lib` is the default) |

macOS looks for OpenSSL under Homebrew (`/opt/homebrew/opt/openssl@3`, `/usr/local/opt/openssl@3`) and SQLite under `/opt/homebrew/opt/sqlite` or `/usr/local/opt/sqlite`.

Static binaries:

```bash
cmake -B build -DENABLE_STATIC_LINKING=ON
cmake --build build
```

## GNU Make

On FreeBSD use `gmake`. The root `Makefile` is a bmake shim that delegates to `GNUmakefile`.

| Target | Action |
|--------|--------|
| `build` | Configure Release and compile |
| `test` | Build then `ctest` |
| `dev-build` / `dev-test` | Debug configure |
| `static-build` / `static-package` | Static link and package |
| `package` | CPack packages for the host OS |
| `package-deb` / `package-rpm` / `package-pkg` / `package-dmg` / `package-msi` / `package-exe` | Platform packages |
| `format` / `check-style` | clang-format when available |
| `clean` | Remove `build/` and `dist/` |

Platform scripts also exist: `scripts/build-linux.sh`, `scripts/build-macos.sh`, `scripts/build-windows.bat`.

## Packaging

CPack and templates under `packaging/` produce DEB/RPM, macOS pkg/dmg, and Windows MSI/NSIS/ZIP. See [packaging/README.md](../../packaging/README.md).

Linux packages install the systemd unit (`/usr/bin/simple-ldapd --config /etc/simple-ldapd/simple-ldapd.conf --foreground`), sysusers/tmpfiles, logrotate, `/etc/simple-ldapd/{templates,examples,schemas}`, and `/usr/share/doc/simple-ldapd/`. They create `/var/lib/simple-ldapd` and `/var/log/simple-ldapd` owned by `simple-ldapd`. They do not start the service.

To keep OpenLDAP's `ldapsearch` on `PATH`:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLDAP_CLI_PREFIX=simple-
cmake --build build
cpack --config build/CPackConfig.cmake
```

## Troubleshooting the build

- **OpenSSL not found on macOS:** install `openssl@3` and re-run CMake so `CMAKE_PREFIX_PATH` picks up Homebrew.
- **SQLite3 warning:** install `sqlite` / `libsqlite3-dev` or set `-DENABLE_SQLITE=OFF` (then `backend = sqlite` fails validation).
- **jsoncpp warning:** safe to ignore; set `-DENABLE_JSON=OFF` to silence it.
- **FreeBSD `make`:** use `gmake`.
