# simple-ldapd

Lightweight LDAPv3 directory daemon for SSO via LDAP bind, with an OpenLDAP-style CLI and Active Directory-friendly schema names.

simple-ldapd is part of [SimpleDaemons](https://github.com/SimpleDaemons). v0.1.0 implements **simple bind and search** over LDAPv3 (memory backend, optional LDIF seed). Write operations, LDAPS, and SASL are still ahead.

## Goals

- Authenticate applications, services, and devices over LDAPv3 simple bind (and later SASL)
- Stay standards-based (RFC 4511 operations, RFC 4519/2798/2307 schemas)
- Offer OpenLDAP-compatible CLI tools: `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`
- Ship AD-like attributes (`sAMAccountName`, `memberOf`, `userAccountControl`) so Windows and Unix clients can search/bind against a familiar tree
- Cross-compile and package for Linux (DEB/RPM), macOS (pkg/dmg), Windows (MSI/NSIS/ZIP), and FreeBSD

## Non-goals (v0.x)

- Kerberos KDC / ticket-based AD SSO (reserved in the roadmap)
- OIDC or SAML (see `simple-oidcd` in the SimpleDaemons future list)
- SMB, Group Policy, or a full Active Directory domain controller

## Status

| Area | State |
|------|--------|
| Build, tests, CPack packaging | Implemented |
| Config, schema registry, memory/LDIF backends | Implemented |
| LDAPv3 BER codec, simple bind, search | Implemented (filter subset) |
| Add / modify / delete | Not implemented |
| LDAPS / StartTLS / SASL | Interfaces only |

## Build

Requires CMake 3.16+, a C++17 compiler, and OpenSSL. jsoncpp is optional.

```bash
# GNU Make (use gmake on FreeBSD)
make build
make test

# or
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Static, self-contained binaries:

```bash
cmake -B build -DENABLE_STATIC_LINKING=ON
cmake --build build
```

Platform scripts: `scripts/build-linux.sh`, `scripts/build-macos.sh`, `scripts/build-windows.bat`.

## Run

Development binds **3389** so root is not required, seeds `config/examples/simple/example.ldif`, and uses root DN `cn=admin,dc=example,dc=com` / password `secret`. Production templates use **389** / **636**.

```bash
./build/simple-ldapd --help
./build/simple-ldapd --version
./build/simple-ldapd --test-config --config config/templates/development.conf
./build/simple-ldapd --foreground --config config/templates/development.conf
```

Search the seeded tree (anonymous or simple bind):

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -b dc=example,dc=com '(objectClass=*)'
```

`ldapadd` / `ldapmodify` / `ldapdelete` / `ldappasswd` still exit with a "not implemented" status.

## Layout

```
include/simple-ldapd/   public headers (core, protocol, schema, backend, auth, security)
src/simple-ldapd/       library sources
main/                   simple-ldapd + OpenLDAP-style CLI tools
schemas/                core, cosine, inetOrgPerson, nis/posix, ad-compat
config/                 examples and templates
packaging/              linux, macos, windows, freebsd
deployment/             systemd, launchd, Windows service, Docker
```

## License

Apache License 2.0. See [LICENSE](LICENSE).
