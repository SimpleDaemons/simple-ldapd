# simple-ldapd

Lightweight LDAPv3 directory daemon for SSO via LDAP bind, with an OpenLDAP-style CLI and Active Directory-friendly schema names.

simple-ldapd is part of [SimpleDaemons](https://github.com/SimpleDaemons). **v1.0.0** is the production contract cut after the 0.x feature series (milestones 1–15). Versions follow [VERSIONING.md](VERSIONING.md).

## 1.0 contract

A small **single-host SSO directory via LDAP bind** — not OpenLDAP-at-scale and not an AD DC.

| Included | Detail |
|----------|--------|
| Bind | Anonymous, simple, SASL PLAIN / DIGEST-MD5 / EXTERNAL / lab GSSAPI |
| Directory | Search, writes (add / modify / delete / modrdn), Compare, Who Am I, paged results |
| Security | LDAPS / StartTLS, ACLs, `{SSHA}` passwords, `log_level`, `bind_rate_limit` |
| Persist | SQLite (`backend = sqlite`); LDIF for lab seed and backup |
| Process model | One process, one host; run under systemd / launchd / a Windows service (`--daemon` does not fork) |
| Limits | `max_pdu_size`, `max_sessions`, `idle_timeout`, `bind_rate_limit` |

**Known limits that stay:** DIGEST-MD5 needs a recoverable password; `{SSHA}` is salted SHA-1; GSSAPI tickets are HMAC lab tickets (not MIT / RFC 4120); SQLite search still loads matching entries in-process; `memberOf` is a static attribute.

## Goals

- Authenticate applications, services, and devices over LDAPv3 (simple bind and SASL)
- Stay standards-based (RFC 4511 operations, RFC 4519/2798/2307 schemas)
- Offer OpenLDAP-compatible CLI tools: `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`, `ldapcompare`, `ldapwhoami`
- Ship AD-like attributes (`sAMAccountName`, `memberOf`, `userAccountControl`) so Windows and Unix clients can search/bind against a familiar tree
- Cross-compile and package for Linux (DEB/RPM), macOS (pkg/dmg), Windows (MSI/NSIS/ZIP), and FreeBSD

## Non-goals

- Kerberos KDC / MIT ticket-based AD SSO (lab GSSAPI tickets are not a KDC)
- OIDC or SAML (see `simple-oidcd` in the SimpleDaemons future list)
- SMB, Group Policy, replication, referrals, overlays, or a full Active Directory domain controller

## Status (v1.0.0)

| Area | State |
|------|--------|
| Build, tests, CPack packaging | Implemented |
| Config, schema registry, memory/LDIF/SQLite backends | Implemented |
| LDAPv3 BER codec, simple bind, search | Implemented (equality, present, substring, and/or/not) |
| Add / modify / delete / modrdn | Implemented (root DN or `acl` write) |
| Schema enforcement on writes | Implemented |
| LDAPS / StartTLS | Implemented |
| SASL | PLAIN, DIGEST-MD5, EXTERNAL (verified client cert), GSSAPI lab tickets |
| Access control | Implemented (`acl` lines; root DN is superuser) |
| `ldappasswd` | Implemented (RFC 3062; stores `{SSHA}`) |
| Compare / Who Am I / paged results | Implemented (`ldapcompare`, `ldapwhoami`, `-E pr=N`) |
| Password storage | `{SSHA}` / `{SHA}` / `{CLEARTEXT}`; `userAccountControl` disable bit |
| SQLite persist | Implemented (`backend = sqlite`; optional LDIF seed) |
| Hardening | `log_level`, `bind_rate_limit`, PDU/session/idle limits, SASL EXTERNAL client certs |

## Documentation

Guides, CLI reference, configuration, diagrams, and production notes: [docs/README.md](docs/README.md).

## Build

Requires CMake 3.16+, a C++17 compiler, OpenSSL, and SQLite3. jsoncpp is optional.

```bash
# GNU Make (use gmake on FreeBSD)
make build
make test

# or
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
# Optional: -DLDAP_CLI_PREFIX=simple-  or  -DCMAKE_INSTALL_PREFIX=/opt/simple-ldapd
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
./build/ldapcompare -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret \
  uid=alice,ou=People,dc=example,dc=com uid:alice
./build/ldapwhoami -H ldap://127.0.0.1:3389 -x -D uid=alice,ou=People,dc=example,dc=com -w alice-secret
```

Writes (add/modify/delete/modrdn) require the root DN unless an `acl` line grants `write` on that subtree. `ldappasswd` can also be used by a bound user to change their own `userPassword`; new values are stored as `{SSHA}`. Seed LDIF plaintext and `{CLEARTEXT}` still bind. `userAccountControl` with bit `0x0002` (typical `514`) disables bind. Simple bind accepts the entry DN, a uid / sAMAccountName, or a DN whose RDN matches that account (so `uid=alice,dc=example,dc=com` still finds `uid=alice,ou=People,dc=example,dc=com`). LDAPS and StartTLS need `enable_ldaps` / `enable_starttls` plus `tls_cert_file` and `tls_key_file`; the high-security template also sets `require_confidentiality` so password binds are refused on cleartext, and `acl = users search *` so anonymous cannot read the tree. SASL GSSAPI needs `gssapi_keytab` (a text lab keytab, not MIT krb5 binary format). SASL DIGEST-MD5 needs a recoverable password, not `{SSHA}`.

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
