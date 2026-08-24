# simple-ldapd project status

**Version:** 0.12.0  
**Status:** Early development (bind/search/writes/TLS/schemas/SASL/ldappasswd/ACLs/hashed passwords)  
**Progress:** ~97%  
**Last updated:** August 2026

## Complete

- CMake + GNU Make + CPack (Linux, macOS, Windows, FreeBSD)
- Daemon listen/accept loop (one session thread per connection) and OpenLDAP-style CLI (`ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete` / `ldappasswd`)
- Config parser (including `root_password` and LDIF seed path)
- Schema registry (OpenLDAP attributetype/objectclass), memory backend, LDIF import/export persist
- LDAPv3 BER codec, simple bind, search, add/modify/delete/modrdn, concurrent sessions, ACLs, hashed passwords
- Schema parser and write-time MUST/MAY/SYNTAX enforcement
- SASL PLAIN, DIGEST-MD5, EXTERNAL, and GSSAPI lab tickets
- RFC 3062 password modify (self-change, root-set, or `acl` write)
- LDAPS listener, StartTLS, and `require_confidentiality` for simple bind
- Unit/integration/security/performance tests
- systemd, launchd, Windows service, Docker, Ansible/Vagrant stubs

## Not started (product)

- MIT Kerberos / a companion KDC (explicitly out of tree)
- Kerberos / AD DC features (explicitly later)
