# Production configuration

Start from the shipped templates; do not use `development.conf` on a public interface.

| Template | Ports | TLS | Extra |
|----------|-------|-----|--------|
| `config/templates/production.conf` | 389 / 636 | LDAPS + StartTLS | LDIF persist, `log_level = info` |
| `config/templates/high-security.conf` | 389 / 636 | LDAPS + StartTLS | `require_confidentiality = true`, `acl = users search *`, `log_level = warning` |

Install paths assumed by those files:

- Config: `/etc/simple-ldapd/` (copy a template to `simple-ldapd.conf`)
- TLS: `/etc/simple-ldapd/tls/server.crt`, `server.key`, `ca.crt`
- Schemas: `/etc/simple-ldapd/schemas`
- Directory: `/var/lib/simple-ldapd/directory.ldif`
- Log: `/var/log/simple-ldapd/simple-ldapd.log`

`root_password` is commented out in the templates on purpose. Set it in a file mode `640` owned by `root:simple-ldapd` (or equivalent). It cannot be rotated with `ldappasswd`.

## Recommended settings

```
listen_address = 0.0.0.0
ldap_port = 389
ldaps_port = 636
enable_ldaps = true
enable_starttls = true
tls_cert_file = /etc/simple-ldapd/tls/server.crt
tls_key_file = /etc/simple-ldapd/tls/server.key
tls_ca_file = /etc/simple-ldapd/tls/ca.crt
backend = ldif
ldif_file = /var/lib/simple-ldapd/directory.ldif
schema_dir = /etc/simple-ldapd/schemas
require_confidentiality = true
foreground = true
```

Keep `foreground = true` when a supervisor starts the process. `foreground = false` only logs that daemonize is unimplemented.

Lab GSSAPI (`gssapi_keytab`) is HMAC tickets for testing, not a replacement for a KDC. Leave it unset in production unless you know you are using the lab mechanism.

## Validate on the host

```bash
simple-ldapd --test-config --config /etc/simple-ldapd/simple-ldapd.conf
```

Full key list: [shared configuration](../shared/configuration/README.md) and [config/README.md](../../config/README.md).
