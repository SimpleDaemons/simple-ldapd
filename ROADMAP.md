# simple-ldapd roadmap

## Milestone 1 — Skeleton — v0.1.0

- Cross-platform CMake / GNU Make build
- Packaging for Linux, macOS, Windows, FreeBSD
- Daemon and OpenLDAP-style CLI entry points
- Pluggable backend interface (memory + LDIF stubs)
- Schema file placeholders and AD-compat names

## Milestone 2 — BER + bind/search (memory) — v0.2.0

- RFC 4511 message codec
- Simple bind against the in-memory tree
- Search with a useful filter subset (`equality`, `and`/`or`/`not`, `present`)
- Anonymous and root-DN binds for labs
- `ldapsearch` talking to a live server

## Milestone 3 — Write ops + LDIF — v0.3.0

- Add / modify / delete / modrdn
- LDIF import/export used as the default file backend
- `ldapadd` / `ldapmodify` / `ldapdelete` talking to a live server

## Milestone 4 — TLS — v0.4.0

- LDAPS (636) and StartTLS
- Certificate and CA configuration
- Require confidentiality for simple bind in the high-security template

## Milestone 5 — Standard + AD-compat schemas — v0.5.0

- Full schema parser (SYNTAX, EQUALITY, SUP, MUST/MAY)
- Enforce schema on writes
- `posixAccount` / `inetOrgPerson` / `sAMAccountName` / `memberOf` used by real clients

## Milestone 6 — SASL — v0.6.0

- PLAIN, DIGEST-MD5, EXTERNAL
- GSSAPI hook (needs a ticket source; not a KDC)

## Milestone 7 — Kerberos integration (later) — v0.7.0 (current)

- Consume tickets for GSSAPI bind
- Optional companion KDC is out of tree unless revisited
