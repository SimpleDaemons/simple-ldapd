# Progress report

v0.5.0 can bind, search, write, encrypt, and enforce schema on the wire. Milestone 6 (SASL) is next as v0.6.0. See [VERSIONING.md](../VERSIONING.md).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept, simple bind (anonymous, root DN, `userPassword`), search with a filter subset, add/modify/delete/modrdn, `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete`, LDAPS/StartTLS, `require_confidentiality`, schema MUST/MAY/SYNTAX on writes, and `ctest`.

What does not work: SASL and `ldappasswd`.
