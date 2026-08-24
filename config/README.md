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
| backend | memory | `memory` or `ldif` |
| ldif_file | | Seed file for the LDIF backend |
| schema_dir | schemas | Directory of `*.schema` files |
| base_dn | dc=example,dc=com | Naming context |
| root_dn | cn=admin,dc=example,dc=com | Directory manager DN |
