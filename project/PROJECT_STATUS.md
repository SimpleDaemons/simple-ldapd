# simple-ldapd project status

**Version:** 0.2.0  
**Status:** Early development (bind/search; writes in progress toward 0.3.0)  
**Progress:** ~25%  
**Last updated:** August 2026

## Complete

- CMake + GNU Make + CPack (Linux, macOS, Windows, FreeBSD)
- Daemon listen/accept loop and OpenLDAP-style `ldapsearch`
- Config parser (including `root_password` and LDIF seed path)
- Schema registry stub, memory backend, LDIF import seed
- LDAPv3 BER codec, simple bind, search filter subset
- Unit/integration/security/performance tests
- systemd, launchd, Windows service, Docker, Ansible/Vagrant stubs

## Not started (product)

- Add / modify / delete / modrdn on the wire
- SASL mechanisms, LDAPS/StartTLS handshakes
- Kerberos / AD DC features (explicitly later)
