# simple-ldapd documentation

Documentation for **simple-ldapd v0.12.0**, a lightweight LDAPv3 directory daemon for SSO via LDAP bind. OpenLDAP-style CLI tools and Active Directory–friendly schema names are included. This is not a domain controller and not a Kerberos KDC.

License: Apache 2.0. Versions follow [VERSIONING.md](../VERSIONING.md).

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

## What works in v0.12.0

- LDAPv3 bind (anonymous, simple, SASL PLAIN / DIGEST-MD5 / EXTERNAL / lab GSSAPI)
- Search with equality, present, substring, and/or/not filters
- Concurrent sessions (one thread per TCP connection)
- Add, modify, delete, and modrdn (root DN or `acl` write)
- Repeatable `acl` search/write rules (`anonymous`, `users`, `dn:`, `group:`)
- RFC 3062 password modify (`ldappasswd`); new passwords stored as `{SSHA}`
- `{SSHA}` / `{SHA}` / `{CLEARTEXT}` `userPassword`; `userAccountControl` disable bit
- LDAPS and StartTLS
- Schema enforcement on writes
- In-memory directory with optional LDIF seed and persist (`ldif_file`)

## Known limits

- DIGEST-MD5 needs a recoverable `userPassword` (`{CLEARTEXT}` or unprefixed); `{SSHA}` is simple-bind only
- GSSAPI tickets are HMAC lab tickets, not MIT Kerberos / RFC 4120
- `stop` / `status` / `reload` and `--daemon` fork are not implemented
- Extensible match and approximate filters are not implemented

See [TECHNICAL_DEBT.md](../project/TECHNICAL_DEBT.md) and the [roadmap](../ROADMAP.md).
