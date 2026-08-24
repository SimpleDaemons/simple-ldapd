# simple-ldapd project status

**Version:** 0.3.0  
**Status:** Early development (bind/search/writes; TLS next as 0.4.0)  
**Progress:** ~40%  
**Last updated:** August 2026

## Complete

- CMake + GNU Make + CPack (Linux, macOS, Windows, FreeBSD)
- Daemon listen/accept loop and OpenLDAP-style CLI (`ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete`)
- Config parser (including `root_password` and LDIF seed path)
- Schema registry stub, memory backend, LDIF import/export persist
- LDAPv3 BER codec, simple bind, search, add/modify/delete/modrdn
- Unit/integration/security/performance tests
- systemd, launchd, Windows service, Docker, Ansible/Vagrant stubs

## Not started (product)

- SASL mechanisms, LDAPS/StartTLS handshakes
- Kerberos / AD DC features (explicitly later)
