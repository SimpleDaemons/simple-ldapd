# Progress report

v0.6.0 can bind (simple and SASL), search, write, encrypt, and enforce schema on the wire. Milestone 7 (Kerberos/GSSAPI tickets) is next as v0.7.0. See [VERSIONING.md](../VERSIONING.md).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept, simple bind, SASL PLAIN / DIGEST-MD5 / EXTERNAL, search with a filter subset, add/modify/delete/modrdn, `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete`, LDAPS/StartTLS, `require_confidentiality`, schema MUST/MAY/SYNTAX on writes, Root DSE SASL discovery, and `ctest`.

What does not work: GSSAPI ticket bind and `ldappasswd`.
