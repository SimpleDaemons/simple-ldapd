# Developer documentation

simple-ldapd is a C++17 library plus several binaries. Public headers live under `include/simple-ldapd/`; sources under `src/simple-ldapd/`; daemon and CLI entry points under `main/`.

## Setup and build

- [Setup](SETUP.md) — clone, dependencies, lab ports
- [Build guide](BUILD_GUIDE.md) — CMake options, Make targets, tests, packaging

## Layout

```
include/simple-ldapd/   public headers
src/simple-ldapd/       library (core, protocol, auth, backend, schema, security)
main/                   simple-ldapd, ldapsearch, ldapadd, ldapmodify, ldapdelete, ldappasswd
tests/unit/             BER, filters, bind, config, backend, ACLs
tests/integration/      live TCP bind/search/write/TLS/SASL/password
schemas/                OpenLDAP-style schema packs
config/templates/       development, production, high-security
```

Namespaces use `simple_ldapd`. Headers use `#pragma once`.

## Conventions

- C++17 only (see `CMAKE_CXX_STANDARD` in `CMakeLists.txt`)
- RAII for sockets, TLS, threads, and file-backed backends
- Protocol codec is RFC 4511 BER; do not pass a temporary `std::vector` into `BerReader` (it stores a pointer)

## Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Integration tests bind `127.0.0.1` with `ldap_port = 0` (ephemeral). They do not need the development template or root.

## Related docs

- [Architecture](../diagrams/architecture.md)
- [Project status](../../project/PROJECT_STATUS.md)
- [Technical debt](../../project/TECHNICAL_DEBT.md)
