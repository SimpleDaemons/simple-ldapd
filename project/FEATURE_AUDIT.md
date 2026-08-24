# Feature audit

**Date:** August 2026

| Feature | Status |
|---------|--------|
| Build / packaging / CI | Implemented |
| Config load/validate | Implemented (key=value) |
| Memory backend | Implemented (in-process, scoped search, rename) |
| LDIF import/export | Implemented (seed + persist; change records in CLI) |
| Schema registry | Implemented (OpenLDAP attributetype/objectclass) |
| Daemon listen | Implemented (accept loop, port 0 for tests) |
| LDAPv3 BER codec | Implemented (bind/search/unbind/add/modify/delete/modrdn) |
| Simple bind | Implemented (anonymous, root DN, `userPassword`) |
| Search filters | Implemented (`equality`, `and`/`or`/`not`, `present`, substring) |
| `ldapsearch` | Implemented |
| Add / modify / delete / modrdn | Implemented (root DN bind) |
| SASL | PLAIN, DIGEST-MD5, EXTERNAL, GSSAPI lab tickets |
| LDAPS / StartTLS | Implemented |
| CLI help/version | Implemented |
| `ldapadd` / `ldapmodify` / `ldapdelete` | Implemented |
| `ldappasswd` | Implemented (RFC 3062) |
| Kerberos KDC | Out of scope |
