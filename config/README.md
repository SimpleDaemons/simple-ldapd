# Configuration files for simple-ldapd

Use `config/templates/` for shipped defaults and `config/examples/` for documented samples.

Key/value syntax (`name = value`). Comments start with `#`.

| Key | Default | Notes |
|-----|---------|-------|
| listen_address | 0.0.0.0 | Bind address |
| ldap_port | 389 | LDAP (use 3389 in development) |
| ldaps_port | 636 | LDAPS |
| enable_ldaps | false | Require cert/key when true |
| enable_starttls | false | StartTLS on the LDAP port |
| tls_cert_file | | Server certificate |
| tls_key_file | | Server private key |
| tls_ca_file | | Optional CA / client trust (required for EXTERNAL and `tls_verify_client`) |
| tls_verify_client | false | Require a client certificate on every TLS handshake |
| backend | memory | `memory`, `ldif`, or `sqlite` |
| ldif_file | | LDIF seed; persist for the LDIF backend; seed-only when sqlite is empty |
| sqlite_file | | Required when `backend = sqlite` |
| schema_dir | schemas | Directory of `*.schema` files |
| base_dn | dc=example,dc=com | Naming context |
| root_dn | cn=admin,dc=example,dc=com | Directory manager DN |
| root_password | | Root DN bind password |
| log_file | | Optional log path |
| log_level | info | `debug`, `info`, `warning`, `error`, `fatal` |
| bind_rate_limit | 0 | Binds per minute per client IP; `0` disables |
| foreground | true | Stay in the foreground |
| require_confidentiality | false | Refuse cleartext password binds |
| krb_realm | from `base_dn` | Lab GSSAPI realm (e.g. EXAMPLE.COM) |
| gssapi_service | ldap/localhost | Service name inside lab tickets |
| gssapi_keytab | | Text lab keytab (`realm` / `service` / `key`) |
| acl | (none) | Repeatable. `WHO PERM [subtree]`. Empty list: anyone may search; only root writes |
