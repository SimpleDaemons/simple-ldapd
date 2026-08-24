# Build guide

Requires CMake 3.16+, a C++17 compiler (Clang, GCC, or MSVC), and OpenSSL. jsoncpp is optional and unused by the current key/value config parser.

## Prerequisites

macOS (Homebrew):

```bash
brew install cmake openssl jsoncpp
```

Debian/Ubuntu:

```bash
sudo apt-get install build-essential cmake pkg-config libssl-dev libjsoncpp-dev
```

FreeBSD:

```bash
pkg install cmake openssl jsoncpp gmake
gmake build
```

Windows: Visual Studio 2019+ with CMake, plus OpenSSL available to CMake.

## CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Binaries land in `build/`: `simple-ldapd`, `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`.

### Options

| Option | Default | Purpose |
|--------|---------|---------|
| `ENABLE_TESTS` | ON | Build `test_ldap_*` and register them with CTest |
| `ENABLE_SSL` | ON | Link OpenSSL; required for LDAPS and StartTLS |
| `ENABLE_JSON` | ON | Link jsoncpp if found (config is still key=value) |
| `ENABLE_PACKAGING` | ON | CPack targets |
| `ENABLE_STATIC_LINKING` | OFF | Self-contained binaries |
| `BUILD_SHARED_LIBS` | OFF | Shared library (static `simple-ldapd_lib` is the default) |

macOS looks for OpenSSL under Homebrew (`/opt/homebrew/opt/openssl@3`, `/usr/local/opt/openssl@3`).

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

## Troubleshooting the build

- **OpenSSL not found on macOS:** install `openssl@3` and re-run CMake so `CMAKE_PREFIX_PATH` picks up Homebrew.
- **jsoncpp warning:** safe to ignore; set `-DENABLE_JSON=OFF` to silence it.
- **FreeBSD `make`:** use `gmake`.
