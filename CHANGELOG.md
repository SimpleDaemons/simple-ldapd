# Changelog

All notable changes to simple-ldapd are documented in this file. Versions follow [VERSIONING.md](VERSIONING.md).

## [Unreleased]

### Added

- (milestone 3) Add / modify / delete / modrdn and write CLI — in progress

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
