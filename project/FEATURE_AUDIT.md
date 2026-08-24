# Feature audit

**Date:** August 2026

| Feature | Status |
|---------|--------|
| Build / packaging / CI | Implemented |
| Config load/validate | Implemented (key=value) |
| Memory backend | Implemented (in-process, scoped search) |
| LDIF import | Partial (seed on startup) |
| Schema registry | Partial (NAME/OID lines) |
| Daemon listen | Implemented (accept loop, port 0 for tests) |
| LDAPv3 BER codec | Implemented (bind/search/unbind) |
| Simple bind | Implemented (anonymous, root DN, `userPassword`) |
| Search filters | Implemented (`equality`, `and`/`or`/`not`, `present`) |
| `ldapsearch` | Implemented |
| Add / modify / delete | Stub (result: unwillingToPerform) |
| SASL | Stub |
| LDAPS / StartTLS | Stub |
| CLI help/version | Implemented |
| `ldapadd` / `ldapmodify` / `ldapdelete` / `ldappasswd` | Stub (exit 2) |
| Kerberos KDC | Out of scope |
