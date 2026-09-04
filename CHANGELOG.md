# Changelog

All notable changes to simple-ldapd are documented in this file. Versions follow [VERSIONING.md](VERSIONING.md).

## [Unreleased]

## [1.0.0] — 2026-09-04

Production contract cut after milestones 1–15.

### Added
- `max_pdu_size`, `max_sessions`, and `idle_timeout` configuration limits
- Production template defaults for PDU/session/idle hardening
- Release checklist and technical debt notes for 1.0

### Changed
- Documented the **1.0 contract** in README and project docs
- systemd / launchd units aligned with install paths and service user
- Dropped leftover early-development / skeleton language

### Notes
- Still no `--daemon` fork; use the OS supervisor for lifecycle
- Known limits stay: DIGEST-MD5 recoverable passwords, lab GSSAPI tickets, static `memberOf`

## [0.15.0] — 2026-08-24

Milestone 15 — Hardening.

### Added

- `log_level` is applied (`debug`, `info`, `warning`, `error`, `fatal`)
- `bind_rate_limit` (binds per minute per client IP; `0` disables); exceeded binds return `busy`
- SASL EXTERNAL requires a verified TLS client certificate; authzid must match the certificate CN
- `tls_verify_client` requires a client certificate on every TLS handshake (`tls_ca_file` required)
- Client `--cert` / `--key` for EXTERNAL; CMake `LDAP_CLI_PREFIX` (e.g. `simple-`) to avoid OpenLDAP name collisions
- Install prefix is no longer forced to `/usr` when `CMAKE_INSTALL_PREFIX` is set
- `automation/ansible/scripts/collect-packages.sh` pulls packages from the four build hosts into `dist/centralized/vVERSION/` (with `SHA256SUMS`) for a GitHub release

### Changed

- systemd unit starts `/usr/bin/simple-ldapd --config /etc/simple-ldapd/simple-ldapd.conf --foreground` as `simple-ldapd` with `CAP_NET_BIND_SERVICE`
- Install and packages create `/var/lib/simple-ldapd` and `/var/log/simple-ldapd` (and `/etc/simple-ldapd/tls`); launchd and the Windows service pass `--config` then `--foreground` so the flag wins over `foreground = false` in the production templates
- CPack DEB/RPM run maintainer scripts (user, ownership, `daemon-reload`) and no longer prompt for a license or start the service
- Ansible build playbooks install `libsqlite3-dev` / `sqlite-devel`, create `/var/lib/simple-ldapd` and `/var/log/simple-ldapd`, and configure CMake with `ENABLE_SQLITE` and `/usr` as the install prefix
- CMake links SQLite with `SQLite3_LIBRARIES` so Debian's `SQLite::SQLite3` find-module target configures
- `make package-all` reads the version from CMake `project()` (currently 0.15.0) and lists binary (`.deb`/`.rpm`) and source (`-src.tar.gz`/`-src.zip`) artifacts separately
- Linux packages install `/etc/simple-ldapd/templates`, `/etc/simple-ldapd/examples`, schemas, and `/usr/share/doc/simple-ldapd/` (docs were previously omitted; configs were landing under `${prefix}/etc`)
- `make package-source` writes a ZIP with `tar` when `zip` is missing (FreeBSD) and does not fail `package-all`

## [0.14.0] — 2026-08-24

Milestone 14 — SQLite backend.

### Added

- `backend = sqlite` with required `sqlite_file` (WAL `entries` / `attributes` tables)
- Optional `ldif_file` seeds an empty SQLite database only; writes do not rewrite that file
- Production and high-security templates use SQLite; LDIF remains the lab and backup format
- `ENABLE_SQLITE` (default ON); missing SQLite3 disables the backend at validate time

## [0.13.0] — 2026-08-24

Milestone 13 — Remaining LDAP ops.

### Added

- Compare (`ldapcompare DN attr:value`); `userPassword` compare verifies `{SSHA}`
- RFC 4532 Who Am I (`ldapwhoami`); Root DSE advertises `1.3.6.1.4.1.4203.1.11.3`
- RFC 2696 paged results (`ldapsearch -E pr=N`)
- Search `typesOnly` (`-A`) and time limit (`-l`); size limit already existed (`-z`)

## [0.12.0] — 2026-08-24

Milestone 12 — Password storage.

### Added

- `{SSHA}` (and `{SHA}`) `userPassword` verify; writes and `ldappasswd` store unsalted plaintext as `{SSHA}`
- `{CLEARTEXT}` and unprefixed values still bind for labs and LDIF seeds
- `userAccountControl` disable bit (`0x0002`, e.g. `514`) fails bind with `invalidCredentials`
- SASL DIGEST-MD5 still needs a recoverable password (`{CLEARTEXT}` or unprefixed)

## [0.11.0] — 2026-08-24

Milestone 11 — Access control.

### Added

- Repeatable `acl = WHO PERM [subtree]` rules (`anonymous`, `users`, `*`, `dn:…`, `group:…`; `search` or `write`)
- Default unchanged when no `acl` lines: anyone may search; only `root_dn` may write
- High-security template: `acl = users search *` (anonymous cannot read the tree)
- Root DN remains superuser; Root DSE stays searchable; self `ldappasswd` still works

## [0.10.0] — 2026-08-24

Milestone 10 — Concurrent connections.

### Added

- Per-connection session threads so overlapping LDAP/LDAPS clients are served together
- Fair accept polling of the LDAP and LDAPS listeners
- Serialized LDIF persist so concurrent writes do not interleave the seed file

## [0.9.0] — 2026-08-24

Milestone 9 — Substring filters.

### Added

- RFC 4511 substring search filters (`initial` / `any` / `final`)
- String parse of `(cn=Ali*)`, `*Exam*`, `*example`, and mixed `A*e`
- BER encode/decode (tag `0xA4`) so `ldapsearch` and other clients work on the wire

## [0.8.1] — 2026-08-24

### Fixed

- Simple bind resolves `uid` / `sAMAccountName` when the bind DN parent does not match the entry (the seeded alice lives under `ou=People`)

## [0.8.0] — 2026-08-24

Milestone 8 — Password modify / `ldappasswd`.

### Added

- RFC 3062 Password Modify extended operation (`1.3.6.1.4.1.4203.1.11.1`)
- Self-service `userPassword` change, or root DN set for another entry
- Working `ldappasswd` (`-s` / `-a` / `-S`)

## [0.7.0] — 2026-08-24

Milestone 7 — Kerberos integration (consume tickets; no in-tree KDC).

### Added

- SASL GSSAPI bind that verifies HMAC lab tickets from `gssapi_keytab`
- Principal mapping to uid / sAMAccountName / DN (`krb_realm`, `gssapi_service`)
- Client `-Y GSSAPI` with `--keytab` or `SIMPLE_LDAPD_KTNAME`

## [0.6.0] — 2026-08-24

Milestone 6 — SASL.

### Added

- SASL PLAIN and DIGEST-MD5 binds (uid / sAMAccountName / DN)
- SASL EXTERNAL over TLS (authzid DN) and a GSSAPI hook that advertises but does not consume tickets
- Root DSE `supportedSASLMechanisms` and `namingContexts`
- Client `-Y` / `-U` for PLAIN and DIGEST-MD5

## [0.5.0] — 2026-08-24

Milestone 5 — Standard + AD-compat schemas.

### Added

- OpenLDAP-style schema parser (NAME, SUP, SYNTAX, EQUALITY, MUST, MAY, SINGLE-VALUE)
- Write-time enforcement for add, modify, and modrdn
- Filled core, cosine, inetOrgPerson, posix, and AD-compat schema files (`sAMAccountName`, `memberOf`, auxiliary `user`)

## [0.4.0] — 2026-08-24

Milestone 4 — TLS.

### Added

- LDAPS listener and StartTLS (`1.3.6.1.4.1.1466.20037`) over OpenSSL
- Certificate / key / CA configuration; `require_confidentiality` for simple bind
- `ldaps://`, `-Z` StartTLS, and `--ca-file` on the OpenLDAP-style clients
- High-security template requires confidentiality so password binds cannot use cleartext

## [0.3.0] — 2026-08-24

Milestone 3 — Write ops + LDIF.

### Added

- Add, modify, delete, and modrdn on the wire (root DN bind required)
- LDIF export/persist for the file backend and change-record parsing
- Working `ldapadd`, `ldapmodify`, and `ldapdelete`

## [0.2.0] — 2026-08-24

Milestone 2 — BER + bind/search.

### Added

- RFC 4511 BER codec for bind, unbind, and search
- Search filters: equality, present, and / or / not
- Simple bind: anonymous, root DN + `root_password`, entry `userPassword`
- TCP accept loop and `ldapsearch` against a live in-memory tree (optional LDIF seed)
- `userPassword` hidden unless bound as the root DN

## [0.1.0] — 2026-08-24

Milestone 1 — Skeleton.

### Added

- C++17 library, daemon CLI, and OpenLDAP-style client tool entry points
- Pluggable memory and LDIF backend stubs
- Schema placeholders (core, cosine, inetOrgPerson, nis, ad-compat)
- Packaging and service files for Linux, macOS, Windows, and FreeBSD
