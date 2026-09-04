# Simple LDAP Daemon — Project Overview

**Project:** `simple-ldapd`  
**Version:** 1.0.0  
**Protocol / role:** LDAPv3 single-host SSO directory  
**Status:** Production-usable — 1.0.0 contract cut after milestones 1–15.

## Where to look

| Document | Role |
|----------|------|
| [ROADMAP.md](ROADMAP.md) | Public plan and milestones |
| [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md) | Item-level checklists |
| [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md) | Verified “what works” |
| [project/PROJECT_STATUS.md](project/PROJECT_STATUS.md) | Metrics and health |
| [CHANGELOG.md](CHANGELOG.md) | Release history |
| [RELEASING.md](RELEASING.md) | How to cut a release |
| [VERSIONING.md](VERSIONING.md) | SemVer policy |

## 1.0 contract (summary)

- Bind (anonymous, simple, SASL PLAIN / DIGEST-MD5 / EXTERNAL / lab GSSAPI), search, writes, TLS, ACLs, `{SSHA}` passwords, SQLite persist, `log_level`, `bind_rate_limit`, PDU/session/idle limits
- One process, one host; run under systemd / launchd / a Windows service (`--daemon` does not fork)
- Known limits: DIGEST-MD5 needs a recoverable password; `{SSHA}` is salted SHA-1; GSSAPI is HMAC lab tickets (not MIT/RFC 4120); `memberOf` is static; no replication / AD DC

## Portfolio

This daemon is part of [SimpleDaemons](https://github.com/SimpleDaemons). Portfolio-wide status lives in the monorepo `PROJECTS_OVERVIEW.md`.

*Last updated: September 2026*
