# Progress report

v0.15.0 can bind (simple and SASL), search, page results, compare, identify the bound DN, serve overlapping clients, apply ACLs, hash `userPassword`, write, persist to SQLite, encrypt, enforce schema, consume GSSAPI lab tickets, change passwords with `ldappasswd`, apply `log_level`, throttle binds, and require a verified client certificate for SASL EXTERNAL. See [VERSIONING.md](../VERSIONING.md).

What works today: `--help` / `--version` / `--test-config`, TCP listen/accept with concurrent sessions, simple bind, SASL PLAIN / DIGEST-MD5 / EXTERNAL / GSSAPI, search with equality / present / substring / and/or/not filters, `acl` search/write rules, `{SSHA}` password storage, `userAccountControl` disable bit, add/modify/delete/modrdn, Compare, Who Am I, RFC 2696 paged results, `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete` / `ldappasswd` / `ldapcompare` / `ldapwhoami`, LDAPS/StartTLS, `require_confidentiality`, schema MUST/MAY/SYNTAX on writes, Root DSE SASL and Password Modify discovery, SQLite (`backend = sqlite`) and LDIF persist, `log_level`, `bind_rate_limit`, and `ctest`.

What does not work: MIT Kerberos tickets and an in-tree KDC.
