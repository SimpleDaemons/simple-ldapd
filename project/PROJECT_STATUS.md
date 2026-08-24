# simple-ldapd project status

**Version:** 0.4.0  
**Status:** Early development (bind/search/writes/TLS; schemas next as 0.5.0)  
**Progress:** ~50%  
**Last updated:** August 2026

## Complete

- CMake + GNU Make + CPack (Linux, macOS, Windows, FreeBSD)
- Daemon listen/accept loop and OpenLDAP-style CLI (`ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete`)
- Config parser (including `root_password` and LDIF seed path)
- Schema registry stub, memory backend, LDIF import/export persist
- LDAPv3 BER codec, simple bind, search, add/modify/delete/modrdn
- LDAPS listener, StartTLS, and `require_confidentiality` for simple bind
- Unit/integration/security/performance tests
- systemd, launchd, Windows service, Docker, Ansible/Vagrant stubs

## Not started (product)

- SASL mechanisms
- Kerberos / AD DC features (explicitly later)
