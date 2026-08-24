# Production

v0.12.0 can bind, search, write, encrypt, enforce schema, apply ACLs, hash passwords, and change passwords, and it serves more than one client at a time. It is still early: lab GSSAPI rather than MIT Kerberos, and no `stop`/`reload` commands.

Use this section when exposing 389/636 on a host, not for the 3389 lab template.

| Document | Contents |
|----------|----------|
| [Configuration](configuration.md) | Production and high-security templates |
| [Deployment](deployment.md) | systemd, launchd, Windows, Docker, packages |
| [Security](security.md) | TLS, bind, SASL, secrets |
| [Operations](operations.md) | Start/stop, backup, updates |
| [Performance](performance.md) | Concurrent sessions and practical sizing |

Shared references: [configuration](../shared/configuration/README.md), [deployment diagrams](../diagrams/deployment.md), [deployment/](../../deployment/README.md).

## Checklist before listen on 389

- Set a strong `root_password` (leave it unset in the file until you do)
- Enable LDAPS or StartTLS with a real certificate
- Prefer `high-security.conf` (`require_confidentiality = true`, `acl = users search *`) if passwords must never cross cleartext and anonymous must not read the tree
- Point `schema_dir` and `ldif_file` at absolute paths owned by the service user
- Run under systemd/launchd/a Windows service (`--daemon` does not fork)
- Open firewall 389/tcp and/or 636/tcp only as needed
