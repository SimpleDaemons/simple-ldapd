# simple-ldapd documentation

Documentation for **simple-ldapd v0.15.0**, a lightweight LDAPv3 directory daemon for SSO via LDAP bind. OpenLDAP-style CLI tools and Active Directory–friendly schema names are included. This is not a domain controller and not a Kerberos KDC.

License: Apache 2.0. Versions follow [VERSIONING.md](../VERSIONING.md). Product promise: [README 1.0 contract](../README.md#10-contract).

## Start here

1. [Quick start](shared/getting-started/quick-start.md) — lab directory on port 3389
2. [Installation](shared/getting-started/installation.md) — build or package
3. [First steps](shared/getting-started/first-steps.md) — bind, search, password change
4. [CLI](shared/user-guide/cli.md) — `simple-ldapd` and the OpenLDAP-style tools
5. [Configuration](shared/configuration/README.md) — key/value reference

## By topic

| Topic | Where |
|-------|--------|
| Search filters | [Search](shared/user-guide/search.md) |
| Architecture and flows | [Diagrams](diagrams/README.md) |
| Examples | [Examples](shared/examples/README.md) |
| Logging | [Observability](shared/observability/README.md) |
| Production | [Production](production/README.md) |
| Security | [Production security](production/security.md) |
| Troubleshooting | [Troubleshooting](shared/troubleshooting/README.md) |
| Building and tests | [Development](development/README.md) |

## 1.0 contract (same as README)

Single-host SSO directory via LDAP bind: anonymous / simple / SASL bind, search, writes, TLS, ACLs, `{SSHA}`, SQLite persist, `log_level`, `bind_rate_limit`. One process per host; supervise with systemd / launchd / Windows service (`--daemon` does not fork).

## What works in v0.15.0

- LDAPv3 bind (anonymous, simple, SASL PLAIN / DIGEST-MD5 / EXTERNAL / lab GSSAPI)
- Search with equality, present, substring, and/or/not filters
- Concurrent sessions (one thread per TCP connection)
- Add, modify, delete, and modrdn (root DN or `acl` write)
- Repeatable `acl` search/write rules (`anonymous`, `users`, `dn:`, `group:`)
- RFC 3062 password modify (`ldappasswd`); new passwords stored as `{SSHA}`
- `{SSHA}` / `{SHA}` / `{CLEARTEXT}` `userPassword`; `userAccountControl` disable bit
- Compare, Who Am I, RFC 2696 paged results, `typesOnly`, search time limit
- LDAPS and StartTLS
- Schema enforcement on writes
- In-memory directory with optional LDIF seed and persist (`ldif_file`)
- SQLite directory (`backend = sqlite`, `sqlite_file`); optional `ldif_file` seed when the database is empty
- Applied `log_level`, per-IP `bind_rate_limit`, SASL EXTERNAL with a verified client certificate

## Known limits

- DIGEST-MD5 needs a recoverable `userPassword` (`{CLEARTEXT}` or unprefixed); `{SSHA}` is salted SHA-1 and simple-bind oriented
- GSSAPI tickets are HMAC lab tickets, not MIT Kerberos / RFC 4120
- SQLite search still loads matching entries in-process; `memberOf` is a static attribute
- `stop` / `status` / `reload` and `--daemon` fork are not implemented (use the OS supervisor)
- Extensible match, approximate, and ordering filters are not implemented

See [TECHNICAL_DEBT.md](../project/TECHNICAL_DEBT.md) and the [roadmap](../ROADMAP.md).
