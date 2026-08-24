# Progress report

v0.3.0 can bind, search, and write on the wire against an in-memory tree (optionally seeded and persisted as LDIF). Milestone 4 (TLS) is next as v0.4.0. See [VERSIONING.md](../VERSIONING.md).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept, simple bind (anonymous, root DN, `userPassword`), search with a filter subset, add/modify/delete/modrdn, `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete`, schema name loading, and `ctest`.

What does not work: LDAPS/StartTLS/SASL, `ldappasswd`, and schema enforcement on writes.
