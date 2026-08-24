# Changelog

All notable changes to simple-ldapd are documented in this file. Versions follow [VERSIONING.md](VERSIONING.md).

## [Unreleased]

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
