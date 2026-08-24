# Configuration

simple-ldapd reads a **key = value** file. Comments start with `#`. There is no JSON/YAML parser in the daemon today (`ENABLE_JSON` only links jsoncpp if present).

Shipped files:

| Path | Use |
|------|-----|
| `config/templates/development.conf` | Lab: 127.0.0.1:3389, memory + LDIF seed |
| `config/templates/production.conf` | 389/636, SQLite persist, TLS on |
| `config/templates/high-security.conf` | Production plus `require_confidentiality` and `acl = users search *` |
| `config/examples/simple/example.ldif` | Seed tree (alice, developers group) |
| `config/examples/simple/lab.keytab` | Text lab GSSAPI keytab |

Canonical key list: [config/README.md](../../../config/README.md). Production notes: [production configuration](../../production/configuration.md).

## Keys

| Key | Default | Notes |
|-----|---------|-------|
| `listen_address` | `0.0.0.0` | Bind address |
| `ldap_port` | `389` | LDAP. Use `3389` in development; `0` in tests (ephemeral) |
| `ldaps_port` | `636` | LDAPS when `enable_ldaps` is true |
| `enable_ldaps` | `false` | Requires `tls_cert_file` and `tls_key_file` |
| `enable_starttls` | `false` | StartTLS on the LDAP port |
| `tls_cert_file` | | Server certificate |
| `tls_key_file` | | Server private key |
| `tls_ca_file` | | Optional CA file. Required for SASL EXTERNAL and `tls_verify_client` |
| `tls_verify_client` | `false` | Require a client certificate on every TLS handshake |
| `backend` | `memory` | `memory`, `ldif`, or `sqlite`. `backend = sqlite` wins even if `ldif_file` is set. Otherwise a non-empty `ldif_file` selects `LdifBackend` even if `backend` is `memory` |
| `ldif_file` | | LDIF seed. For `ldif`, successful writes persist back to this path. For `sqlite`, seed only when the database is empty |
| `sqlite_file` | | Required when `backend = sqlite` |
| `schema_dir` | `schemas` | Directory of `*.schema` files; relative to cwd |
| `base_dn` | `dc=example,dc=com` | Naming context advertised on the Root DSE |
| `root_dn` | `cn=admin,dc=example,dc=com` | Directory manager |
| `root_password` | | Root bind password (not changeable via `ldappasswd`) |
| `log_file` | | Optional log path; otherwise stderr |
| `log_level` | `info` | `debug`, `info`, `warning`, `error`, `fatal`; applied to the logger |
| `bind_rate_limit` | `0` | Binds per minute per client IP; `0` unlimited. Exceeded binds return `busy` |
| `foreground` | `true` | `--daemon` sets this false but does not fork |
| `require_confidentiality` | `false` | Refuse cleartext password binds |
| `krb_realm` | derived from `base_dn` | Lab GSSAPI realm (e.g. `EXAMPLE.COM`) |
| `gssapi_service` | `ldap/localhost` | Service name inside lab tickets |
| `gssapi_keytab` | | Text lab keytab (`realm` / `service` / `key`) |
| `acl` | (none) | Repeatable `WHO PERM [subtree]` |

## Access control

Repeatable `acl` lines. WHO is `anonymous`, `users` (any non-empty bind DN), `*` / `anyone`, `dn:…`, or `group:…`. PERM is `search` or `write` (`write` includes search on that subtree). Subtree is a DN, or `*` / omitted for the whole tree.

```
acl = users search dc=example,dc=com
acl = dn:uid=alice,ou=People,dc=example,dc=com write ou=People,dc=example,dc=com
acl = group:cn=directory-admins,ou=Groups,dc=example,dc=com write dc=example,dc=com
```

Group membership uses `member` / `uniqueMember` DNs, `memberUid`, or the bound entry's `memberOf`.

With **no** `acl` lines, anyone may search and only `root_dn` may write (previous behavior). With one or more lines, unmatched access is denied. `root_dn` is always superuser. The Root DSE (empty search base) stays searchable. Self-service `ldappasswd` does not need a write ACL.

## Validate

```bash
./build/simple-ldapd --test-config --config config/templates/development.conf
```

`--test-config` (or command `test`) runs `validateDetailed` and exits. TLS enabled without cert/key fails validation.

## Schema

Every `*.schema` file in `schema_dir` is loaded at start. Packs shipped in [schemas/](../../../schemas/README.md): core, cosine, inetOrgPerson, nis/posix, ad-compat. Writes are checked against MUST/MAY/SYNTAX; search is not.

## Backends

`LdapDaemon` constructs `SqliteBackend` when `backend = sqlite` (requires `sqlite_file`; fails validation if SQLite was not built). Otherwise it constructs `LdifBackend` when `ldif_file` is non-empty **or** `backend = ldif`. Otherwise it uses `MemoryBackend` (in-process tree, no persist).

`SqliteBackend` stores entries in WAL SQLite. Each successful add/modify/delete/modrdn/password-modify is committed immediately (`persist()` is a no-op). Optional `ldif_file` imports content records only when the database has zero entries. Writes never rewrite `ldif_file`; use `exportFile` (or a stopped-process copy of `sqlite_file`) for backups.

`LdifBackend` is a memory tree that imports `ldif_file` at start and writes the whole file on successful add/modify/delete/modrdn/password-modify. The development template sets `backend = memory` and `ldif_file = config/examples/simple/example.ldif`, so it still persists — do not commit a mutated seed.

Keep `schema_dir`, `ldif_file`, and `sqlite_file` readable (and writable, for persist) by the process user. Relative paths follow the current working directory.
