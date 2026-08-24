# Versioning

simple-ldapd uses [Semantic Versioning](https://semver.org/) on the **0.x** series. Each completed [roadmap](ROADMAP.md) milestone is a **minor** bump. Fixes and docs inside a milestone are **patch** bumps. 1.0.0 is reserved until a durable backend and hardening land on top of bind, search, writes, TLS, concurrent sessions, ACLs, hashed passwords, and remaining LDAP ops (see [ROADMAP.md](ROADMAP.md)).

Git tags and GitHub Releases use the `vMAJOR.MINOR.PATCH` form and point at the commit that finished that version.

## Milestone map

| Version | Milestone | What it means | Status |
|---------|-----------|---------------|--------|
| **0.1.0** | 1 — Skeleton | Build, packaging, CLI stubs, schema/config placeholders | Released (`v0.1.0`) |
| **0.2.0** | 2 — BER + bind/search | Live LDAPv3 simple bind and search, `ldapsearch` | Released (`v0.2.0`) |
| **0.3.0** | 3 — Write ops + LDIF | Add / modify / delete / modrdn, LDIF persist, write CLI | Released (`v0.3.0`) |
| **0.4.0** | 4 — TLS | LDAPS and StartTLS | Released (`v0.4.0`) |
| **0.5.0** | 5 — Schemas | Enforce core/cosine/inetOrgPerson/posix/AD-compat | Released (`v0.5.0`) |
| **0.6.0** | 6 — SASL | PLAIN, DIGEST-MD5, EXTERNAL, GSSAPI hook | Released (`v0.6.0`) |
| **0.7.0** | 7 — Kerberos | Consume tickets for GSSAPI bind (no in-tree KDC) | Released (`v0.7.0`) |
| **0.8.0** | 8 — Password modify | RFC 3062 and `ldappasswd` | Released (`v0.8.0`) |
| **0.9.0** | 9 — Substring filters | RFC 4511 `initial` / `any` / `final` search | Released (`v0.9.0`) |
| **0.10.0** | 10 — Concurrent connections | More than one client at a time | Released (`v0.10.0`) |
| **0.11.0** | 11 — Access control | Subtree search/write ACLs | Released (`v0.11.0`) |
| **0.12.0** | 12 — Password storage | Hashed `userPassword`, disabled accounts | Released (`v0.12.0`) |
| **0.13.0** | 13 — Remaining LDAP ops | Compare, Who Am I, paged results | Released (`v0.13.0`) |
| **0.14.0** | 14 — SQLite backend | Durable store without full-LDIF rewrite | Planned |
| **0.15.0** | 15 — Hardening | log level, rate limit, EXTERNAL certs | Planned |
| **1.0.0** | Production-usable SSO directory | After 14–15 (durable store and hardening) | Reserved |

## Rules

- Do not retcon a released tag. If 0.2.0 already shipped, a bind/search bugfix is **0.2.1**, not another 0.2.0.
- `include/simple-ldapd/version.hpp` (`kVersion`) and `CMakeLists.txt` (`project(... VERSION ...)`) stay in lockstep with the tag.
- `CHANGELOG.md` has a section per released version. Work toward the next milestone lives under **Unreleased** until that tag is cut.
- Cutting a release: update version files and changelog, commit, `git tag -a vX.Y.Z`, push the tag, create the GitHub Release from that tag.
