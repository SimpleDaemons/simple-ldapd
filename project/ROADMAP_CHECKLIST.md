# simple-ldapd — Roadmap Checklist

**Current version:** 1.0.0  
**Overall progress:** Milestones 1–15 released; **1.0.0** production contract cut  
**Honest assessment:** Prefer [PROGRESS_REPORT.md](PROGRESS_REPORT.md). Public plan: [ROADMAP.md](../ROADMAP.md). Versions: [VERSIONING.md](../VERSIONING.md).

**Product line:** Production (Apache 2.0) — single-host SSO directory via LDAP bind (not OpenLDAP-at-scale, not an AD DC).

---

## Milestone 1 — Skeleton — v0.1.0

**Status:** ✅ Released

### Build & packaging
- [x] Cross-platform CMake / GNU Make build
- [x] Packaging for Linux, macOS, Windows, FreeBSD
- [x] Daemon and OpenLDAP-style CLI entry points
- [x] Pluggable backend interface (memory + LDIF stubs)
- [x] Schema file placeholders and AD-compat names

---

## Milestone 2 — BER + bind/search (memory) — v0.2.0

**Status:** ✅ Released

### Protocol
- [x] RFC 4511 message codec (BER)
- [x] Simple bind against the in-memory tree
- [x] Search filter subset: equality, and/or/not, present
- [x] Anonymous and root-DN binds for labs
- [x] `ldapsearch` against a live server

---

## Milestone 3 — Write ops + LDIF — v0.3.0

**Status:** ✅ Released

- [x] Add / modify / delete / modrdn
- [x] LDIF import/export as file backend
- [x] `ldapadd` / `ldapmodify` / `ldapdelete` against a live server

---

## Milestone 4 — TLS — v0.4.0

**Status:** ✅ Released

- [x] LDAPS (636) listener
- [x] StartTLS
- [x] Certificate and CA configuration
- [x] `require_confidentiality` for simple bind (high-security template)

---

## Milestone 5 — Standard + AD-compat schemas — v0.5.0

**Status:** ✅ Released

- [x] Schema parser (SYNTAX, EQUALITY, SUP, MUST/MAY)
- [x] Enforce schema on writes
- [x] `posixAccount` / `inetOrgPerson` / `sAMAccountName` / `memberOf` usable by clients

---

## Milestone 6 — SASL — v0.6.0

**Status:** ✅ Released

- [x] SASL PLAIN
- [x] SASL DIGEST-MD5
- [x] SASL EXTERNAL
- [x] GSSAPI hook (ticket source required; not a KDC)

---

## Milestone 7 — Kerberos integration — v0.7.0

**Status:** ✅ Released (lab tickets)

- [x] Consume tickets for GSSAPI bind
- [x] Document: companion KDC out of tree / lab HMAC tickets (not MIT / RFC 4120)

---

## Milestone 8 — Password modify — v0.8.0

**Status:** ✅ Released

- [x] RFC 3062 Password Modify extended operation
- [x] `ldappasswd` for self-change and root-set passwords

---

## Milestone 9 — Substring filters — v0.9.0

**Status:** ✅ Released

- [x] RFC 4511 substring filters (`initial` / `any` / `final`)
- [x] String parse, BER encode/decode
- [x] Live `ldapsearch` coverage

---

## Milestone 10 — Concurrent connections — v0.10.0

**Status:** ✅ Released

- [x] Serve more than one LDAP client at a time
- [x] Per-connection TLS / StartTLS unchanged
- [x] Fair accept between LDAP and LDAPS listeners

---

## Milestone 11 — Access control — v0.11.0

**Status:** ✅ Released

- [x] Configured ACLs for search vs write on a subtree (bind DN or group)
- [x] Root DN remains superuser
- [x] Anonymous search can be limited (`acl = users search *` in high-security template)

---

## Milestone 12 — Password storage — v0.12.0

**Status:** ✅ Released

- [x] Store `userPassword` as `{SSHA}` (still accept `{CLEARTEXT}` for labs)
- [x] Honor `userAccountControl` disabled bit on bind
- [x] Keep `root_password` in config (not an entry)
- [x] Document DIGEST-MD5 recoverable-password requirement

---

## Milestone 13 — Remaining LDAP ops — v0.13.0

**Status:** ✅ Released

- [x] Compare
- [x] Who Am I (`1.3.6.1.4.1.4203.1.11.3`)
- [x] Paged results (RFC 2696)
- [x] Search `typesOnly` and time limit (size limit already worked)

---

## Milestone 14 — SQLite backend — v0.14.0

**Status:** ✅ Released

- [x] Durable SQLite store (`backend = sqlite`)
- [x] LDIF import/export retained for lab seed and backup

---

## Milestone 15 — Hardening — v0.15.0

**Status:** ✅ Released

- [x] Apply `log_level`
- [x] Bind rate limiter (`bind_rate_limit`)
- [x] SASL EXTERNAL requires verified client certificate before trusting authzid
- [x] Optional install prefix / `LDAP_CLI_PREFIX` to avoid OpenLDAP name collisions

---

## 1.0.0 cut — production-usable SSO directory

**Status:** 🔄 In progress (hygiene / contract)

### Contract (docs at tag time)
- [ ] Write 1.0 contract into README / docs / CHANGELOG
- [ ] Drop “early development” / “skeleton” language in status docs
- [ ] Refresh SimpleDaemons `docs/FUTURE_DAEMONS.md` blurb for simple-ldapd

### Packaging (production templates)
- [x] Create `/var/lib/simple-ldapd` and `/var/log/simple-ldapd` (or platform equivalents)
- [x] systemd / launchd / Windows unit paths match installed binary and config
- [x] Document `-DLDAP_CLI_PREFIX=simple-` and private `CMAKE_INSTALL_PREFIX`

### Optional polish (do not block 1.0 unless public-389 bar)
- [ ] Max LDAP PDU size (bounded BER length)
- [ ] Max concurrent sessions and/or idle timeout
- [ ] CI workflow that builds and runs `ctest`

### Explicitly out of 1.0
- [x] Ordering / approximate / extensible match filters — **Later**
- [x] Computed `memberOf` — **Later**
- [x] MIT Kerberos / RFC 4120 GSSAPI — **Later / out of tree**
- [x] Replication, referrals, overlays, KDC, AD DC — **Out of scope**
- [x] `--daemon` fork and `stop` / `status` / `reload` — **Out of scope** (use OS supervisor)

---

## Later (not scheduled)

- [ ] Ordering / approximate / extensible match filters
- [ ] Computed `memberOf` from group membership
- [ ] MIT Kerberos / RFC 4120 GSSAPI
- [ ] Replication, referrals, overlays
- [ ] `--daemon` fork and supervisor CLI verbs

---

*Last updated: August 2026*
