# Production notes

v0.1.0 is not production-ready. Use the development template on an unprivileged port (3389) until the LDAPv3 codec lands.

When the daemon is functional, prefer:

- `config/templates/production.conf` or `high-security.conf`
- systemd (`deployment/systemd/simple-ldapd.service`)
- launchd (`deployment/launchd/com.simpledaemons.simple-ldapd.plist`)
- LDAPS on 636 with a real certificate
