# simple-ldapd project status

**Version:** 0.6.0  
**Status:** Early development (bind/search/writes/TLS/schemas/SASL; GSSAPI tickets next as 0.7.0)  
**Progress:** ~75%  
**Last updated:** August 2026

## Complete

- CMake + GNU Make + CPack (Linux, macOS, Windows, FreeBSD)
- Daemon listen/accept loop and OpenLDAP-style CLI (`ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete`)
- Config parser (including `root_password` and LDIF seed path)
- Schema registry (OpenLDAP attributetype/objectclass), memory backend, LDIF import/export persist
- LDAPv3 BER codec, simple bind, search, add/modify/delete/modrdn
- Schema parser and write-time MUST/MAY/SYNTAX enforcement
- SASL PLAIN, DIGEST-MD5, EXTERNAL, and advertised GSSAPI
- LDAPS listener, StartTLS, and `require_confidentiality` for simple bind
- Unit/integration/security/performance tests
- systemd, launchd, Windows service, Docker, Ansible/Vagrant stubs

## Not started (product)

- GSSAPI ticket consumption (needs an out-of-tree KDC / v0.7.0)
- Kerberos / AD DC features (explicitly later)
