# Progress report

v0.8.0 can bind (simple and SASL), search, write, encrypt, enforce schema, consume GSSAPI lab tickets, and change passwords with `ldappasswd`. See [VERSIONING.md](../VERSIONING.md).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept, simple bind, SASL PLAIN / DIGEST-MD5 / EXTERNAL / GSSAPI, search with a filter subset, add/modify/delete/modrdn, `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete` / `ldappasswd`, LDAPS/StartTLS, `require_confidentiality`, schema MUST/MAY/SYNTAX on writes, Root DSE SASL and Password Modify discovery, and `ctest`.

What does not work: MIT Kerberos tickets and an in-tree KDC.
