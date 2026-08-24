# Progress report

v0.7.0 can bind (simple and SASL, including GSSAPI lab tickets), search, write, encrypt, and enforce schema on the wire. Roadmap milestones 1–7 are tagged. See [VERSIONING.md](../VERSIONING.md).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept, simple bind, SASL PLAIN / DIGEST-MD5 / EXTERNAL / GSSAPI, search with a filter subset, add/modify/delete/modrdn, `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete`, LDAPS/StartTLS, `require_confidentiality`, schema MUST/MAY/SYNTAX on writes, Root DSE SASL discovery, and `ctest`.

What does not work: MIT Kerberos tickets, an in-tree KDC, and `ldappasswd`.
