# simple-ldapd project status

**Version:** 0.1.0  
**Status:** Skeleton / early development  
**Progress:** ~10% (infrastructure only)  
**Last updated:** August 2026

## Complete

- CMake + GNU Make + CPack (Linux, macOS, Windows, FreeBSD)
- Daemon and OpenLDAP-style CLI binaries that parse flags
- Config parser, schema registry stub, memory/LDIF backends
- Unit/integration/security/performance smoke tests
- systemd, launchd, Windows service, Docker, Ansible/Vagrant stubs

## Not started (product)

- LDAPv3 BER codec and operations
- Real simple-bind password verification
- SASL mechanisms, LDAPS/StartTLS handshakes
- Kerberos / AD DC features (explicitly later)
