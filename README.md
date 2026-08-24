# simple-ldapd

Lightweight LDAPv3 directory daemon for SSO via LDAP bind, with an OpenLDAP-style CLI and Active Directory-friendly schema names.

simple-ldapd is part of [SimpleDaemons](https://github.com/SimpleDaemons). **v0.11.0** implements simple bind, SASL, search, concurrent sessions, ACLs, directory writes, TLS, schema enforcement, GSSAPI lab tickets, and `ldappasswd`. Versions follow [VERSIONING.md](VERSIONING.md).

## Goals

- Authenticate applications, services, and devices over LDAPv3 simple bind (and later SASL)
- Stay standards-based (RFC 4511 operations, RFC 4519/2798/2307 schemas)
- Offer OpenLDAP-compatible CLI tools: `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`
- Ship AD-like attributes (`sAMAccountName`, `memberOf`, `userAccountControl`) so Windows and Unix clients can search/bind against a familiar tree
- Cross-compile and package for Linux (DEB/RPM), macOS (pkg/dmg), Windows (MSI/NSIS/ZIP), and FreeBSD

## Non-goals (v0.x)

- Kerberos KDC / MIT ticket-based AD SSO (reserved; lab GSSAPI tickets are not a KDC)
- OIDC or SAML (see `simple-oidcd` in the SimpleDaemons future list)
- SMB, Group Policy, or a full Active Directory domain controller

## Status

| Area | State |
|------|--------|
| Build, tests, CPack packaging | Implemented |
| Config, schema registry, memory/LDIF backends | Implemented |
| LDAPv3 BER codec, simple bind, search | Implemented (equality, present, substring, and/or/not) |
| Add / modify / delete / modrdn | Implemented (root DN or `acl` write) |
| Schema enforcement on writes | Implemented |
| LDAPS / StartTLS | Implemented |
| SASL | PLAIN, DIGEST-MD5, EXTERNAL, GSSAPI lab tickets |
| Access control | Implemented (`acl` lines; root DN is superuser) |
| `ldappasswd` | Implemented (RFC 3062) |

## Documentation

Guides, CLI reference, configuration, diagrams, and production notes: [docs/README.md](docs/README.md).

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
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(cn=Ali*)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -b dc=example,dc=com '(objectClass=*)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y PLAIN -U alice -w alice-secret -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y DIGEST-MD5 -U alice -w alice-secret -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y GSSAPI -U alice --keytab config/examples/simple/lab.keytab -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldaps://127.0.0.1:6636 -x -D cn=admin,dc=example,dc=com -w secret -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Z --ca-file tls/ca.crt -x -D cn=admin,dc=example,dc=com -w secret -b dc=example,dc=com '(uid=alice)'
./build/ldapadd -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -f change.ldif
./build/ldapmodify -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -f change.ldif
./build/ldapdelete -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret 'uid=bob,ou=People,dc=example,dc=com'
./build/ldappasswd -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -s alice-new uid=alice,ou=People,dc=example,dc=com
./build/ldappasswd -H ldap://127.0.0.1:3389 -x -D uid=alice,ou=People,dc=example,dc=com -w alice-secret -s alice-newer
```

Writes (add/modify/delete/modrdn) require the root DN unless an `acl` line grants `write` on that subtree. `ldappasswd` can also be used by a bound user to change their own `userPassword`. Simple bind accepts the entry DN, a uid / sAMAccountName, or a DN whose RDN matches that account (so `uid=alice,dc=example,dc=com` still finds `uid=alice,ou=People,dc=example,dc=com`). LDAPS and StartTLS need `enable_ldaps` / `enable_starttls` plus `tls_cert_file` and `tls_key_file`; the high-security template also sets `require_confidentiality` so password binds are refused on cleartext, and `acl = users search *` so anonymous cannot read the tree. SASL GSSAPI needs `gssapi_keytab` (a text lab keytab, not MIT krb5 binary format).

## Layout

```
include/simple-ldapd/   public headers (core, protocol, schema, backend, auth, security)
src/simple-ldapd/       library sources
main/                   simple-ldapd + OpenLDAP-style CLI tools
schemas/                core, cosine, inetOrgPerson, nis/posix, ad-compat
config/                 examples and templates
docs/                   getting started, CLI, architecture, production
packaging/              linux, macos, windows, freebsd
deployment/             systemd, launchd, Windows service, Docker
```

## License

Apache License 2.0. See [LICENSE](LICENSE).
