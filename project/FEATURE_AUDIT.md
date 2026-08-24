# Feature audit

**Date:** August 2026

| Feature | Status |
|---------|--------|
| Build / packaging / CI | Implemented |
| Config load/validate | Implemented (key=value) |
| Memory backend | Implemented (in-process, scoped search, rename) |
| SQLite backend | Implemented (WAL persist; optional LDIF seed when empty) |
| LDIF import/export | Implemented (lab seed + persist; change records in CLI; backup export) |
| Schema registry | Implemented (OpenLDAP attributetype/objectclass) |
| Daemon listen | Implemented (concurrent session threads, port 0 for tests) |
| LDAPv3 BER codec | Implemented (bind/search/unbind/add/modify/delete/modrdn/compare) |
| Simple bind | Implemented (anonymous, root DN, `{SSHA}` / `{CLEARTEXT}` `userPassword`) |
| Search filters | Implemented (`equality`, `and`/`or`/`not`, `present`, substring) |
| `ldapsearch` | Implemented (paged `-E pr=N`, typesOnly `-A`, `-l`/`-z`) |
| Add / modify / delete / modrdn | Implemented (root DN or `acl` write) |
| SASL | PLAIN, DIGEST-MD5, EXTERNAL (verified client cert), GSSAPI lab tickets |
| LDAPS / StartTLS | Implemented |
| Access control | Implemented (`acl` WHO/PERM/subtree) |
| Bind rate limit | Implemented (`bind_rate_limit` per client IP) |
| CLI help/version | Implemented |
| `ldapadd` / `ldapmodify` / `ldapdelete` | Implemented |
| `ldapcompare` / `ldapwhoami` | Implemented |
| `ldappasswd` | Implemented (RFC 3062; stores `{SSHA}`) |
| Password storage | `{SSHA}` / `{SHA}` / `{CLEARTEXT}`; `userAccountControl` disable bit |
| Kerberos KDC | Out of scope |
