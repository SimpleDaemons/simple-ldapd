# Progress report

v0.2.0 can bind and search on the wire against an in-memory tree (optionally seeded from LDIF). Milestone 3 (writes / LDIF persist) is next as v0.3.0. See [VERSIONING.md](../VERSIONING.md).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept, simple bind (anonymous, root DN, `userPassword`), search with a filter subset, `ldapsearch`, schema name loading, and `ctest`.

What does not work: add/modify/delete, LDAPS/StartTLS/SASL, and the remaining CLI write tools (they still exit 2).
