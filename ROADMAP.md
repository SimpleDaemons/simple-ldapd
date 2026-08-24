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

## Milestone 7 — Kerberos integration — v0.7.0

- Consume tickets for GSSAPI bind
- Optional companion KDC is out of tree unless revisited

## Milestone 8 — Password modify — v0.8.0

- RFC 3062 Password Modify extended operation
- `ldappasswd` for self-change and root-set passwords

## Milestone 9 — Substring filters — v0.9.0

- RFC 4511 substring filters (`initial` / `any` / `final`)
- String parse, BER encode/decode, and live `ldapsearch`

## Milestone 10 — Concurrent connections — v0.10.0

- Serve more than one LDAP client at a time
- Per-connection TLS / StartTLS unchanged
- Keep accept fair between the LDAP and LDAPS listeners

## Milestone 11 — Access control — v0.11.0

- Configured ACLs for search vs write on a subtree (bind DN or group)
- Root DN remains the superuser
- Anonymous search can be limited (`acl = users search *` in the high-security template)

## Milestone 12 — Password storage — v0.12.0

- Store `userPassword` hashed (`{SSHA}`), still accept `{CLEARTEXT}` for labs
- Honor `userAccountControl` disabled bit on bind
- Keep `root_password` in config (not an entry)
- SASL DIGEST-MD5 requires a recoverable password (`{CLEARTEXT}` or unprefixed)

## Milestone 13 — Remaining LDAP ops — v0.13.0

- Compare
- Who Am I (`1.3.6.1.4.1.4203.1.11.3`)
- Paged results (RFC 2696) so large trees do not dump in one response
- Search `typesOnly` and time limit (size limit already works)

## Milestone 14 — SQLite backend — v0.14.0

- Durable store without rewriting the whole LDIF on every write
- Keep LDIF import/export as the lab and backup format

## Milestone 15 — Hardening — v0.15.0

- Apply `log_level` (parsed today, unused)
- Bind rate limiter (stub exists)
- SASL EXTERNAL: require a verified client certificate before trusting authzid
- Optional install prefix / binary names that do not collide with OpenLDAP

## 1.0.0 cut — production-usable SSO directory

Milestones 1–15 are released. **1.0.0** is a hygiene and contract cut on top of that series, not a new LDAP feature set. It means a small **single-host SSO directory via LDAP bind**, not OpenLDAP-at-scale and not an AD DC.

### Contract (write this into README / docs / CHANGELOG at tag time)

- Bind (anonymous, simple, SASL PLAIN / DIGEST-MD5 / EXTERNAL / lab GSSAPI), search, writes, TLS, ACLs, `{SSHA}` passwords, SQLite persist, `log_level`, `bind_rate_limit`
- One process, one host; run under systemd / launchd / a Windows service (`--daemon` does not fork)
- Known limits that stay: DIGEST-MD5 needs a recoverable password; `{SSHA}` is salted SHA-1; GSSAPI tickets are HMAC lab tickets, not MIT / RFC 4120; SQLite search still loads matching entries in-process; `memberOf` is a static attribute
- Drop “early development” / “skeleton” language; refresh the `simple-ldapd` blurb in SimpleDaemons `docs/FUTURE_DAEMONS.md`

### Packaging (must match production templates)

- [x] Create `/var/lib/simple-ldapd` and `/var/log/simple-ldapd` (or platform equivalents) with the service user
- [x] systemd / launchd / Windows unit: `ExecStart` path and `--foreground --config` must match the installed binary and `/etc/simple-ldapd/simple-ldapd.conf`
- [x] Document `-DLDAP_CLI_PREFIX=simple-` (and a private `CMAKE_INSTALL_PREFIX`) so OpenLDAP tool names do not collide if both are on `PATH`

### Optional polish (do not block 1.0 unless “safe on a public 389” is the bar)

- Max LDAP PDU size so a BER length cannot grow without bound
- Max concurrent sessions and/or idle timeout (today: one thread per connection, no cap)
- A CI workflow that builds and runs `ctest` (the feature audit overstates this today)

### Do not pull into 1.0

Items under **Later** and **Out of scope**. A 1.1+ can add them without retconning 1.0.

## Later (not scheduled)

- Ordering / approximate / extensible match filters
- Computed `memberOf` (today it is a static attribute)
- MIT Kerberos / RFC 4120 GSSAPI (lab HMAC tickets stay)
- Replication, referrals, overlays, a KDC, or an AD DC
- `--daemon` fork and `stop` / `status` / `reload` (use the OS supervisor)

## Out of scope (1.0)

- In-tree Kerberos KDC, SMB, Group Policy
- OIDC or SAML (see `simple-oidcd`)
