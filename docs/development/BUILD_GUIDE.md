# Build guide

## Prerequisites

- CMake >= 3.16
- C++17 compiler (Clang, GCC, or MSVC)
- OpenSSL
- Optional: jsoncpp, cppcheck

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

Windows: Visual Studio 2019+ with CMake, plus OpenSSL.

## Targets

| Make target | Action |
|-------------|--------|
| `build` | Configure and compile |
| `test` | `ctest` |
| `package` | CPack packages for the host OS |
| `static-package` | Static link + package |
| `clean` | Remove `build/` and `dist/` |

On FreeBSD use `gmake`. The root `Makefile` is a bmake shim that delegates to `GNUmakefile`.
