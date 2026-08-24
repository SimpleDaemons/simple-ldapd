# Progress report

v0.4.0 can bind, search, and write on the wire, including LDAPS and StartTLS. Milestone 5 (schemas) is next as v0.5.0. See [VERSIONING.md](../VERSIONING.md).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept, simple bind (anonymous, root DN, `userPassword`), search with a filter subset, add/modify/delete/modrdn, `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete`, LDAPS/StartTLS, `require_confidentiality`, schema name loading, and `ctest`.

What does not work: SASL, `ldappasswd`, and schema enforcement on writes.
