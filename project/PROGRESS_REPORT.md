# Progress report

v0.1.0 can bind and search on the wire against an in-memory tree (optionally seeded from LDIF).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept, simple bind (anonymous, root DN, `userPassword`), search with a filter subset, `ldapsearch`, schema name loading, and `ctest`.

What does not work: add/modify/delete, LDAPS/StartTLS/SASL, and the remaining CLI write tools (they still exit 2).
