# Versioning

simple-ldapd uses [Semantic Versioning](https://semver.org/) on the **0.x** series. Each completed [roadmap](ROADMAP.md) milestone is a **minor** bump. Fixes and docs inside a milestone are **patch** bumps. 1.0.0 is reserved until bind, search, writes, and TLS are all production-usable.

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
| **0.7.0** | 7 — Kerberos | Consume tickets for GSSAPI bind (no in-tree KDC) | Planned |

## Rules

- Do not retcon a released tag. If 0.2.0 already shipped, a bind/search bugfix is **0.2.1**, not another 0.2.0.
- `include/simple-ldapd/version.hpp` (`kVersion`) and `CMakeLists.txt` (`project(... VERSION ...)`) stay in lockstep with the tag.
- `CHANGELOG.md` has a section per released version. Work toward the next milestone lives under **Unreleased** until that tag is cut.
- Cutting a release: update version files and changelog, commit, `git tag -a vX.Y.Z`, push the tag, create the GitHub Release from that tag.
