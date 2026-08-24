# Changelog

## 0.1.0 — 2026-08-24

- LDAPv3 BER codec with simple bind, unbind, and search (equality / present / and / or / not)
- In-memory directory (optional LDIF seed) and working `ldapsearch`
- Anonymous, root-DN, and `userPassword` binds; `userPassword` hidden unless bound as root
- Initial project skeleton: C++17 library, daemon CLI, OpenLDAP-style client tools
- Schema placeholders (core, cosine, inetOrgPerson, nis, ad-compat)
- Packaging and service files for Linux, macOS, Windows, and FreeBSD
