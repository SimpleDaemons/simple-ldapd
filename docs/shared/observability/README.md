# Observability

simple-ldapd logs text lines. There is no Prometheus endpoint, tracing, or SNMP.

## Log destinations

| Setting | Behavior |
|---------|----------|
| `log_file` empty | stderr (captured by systemd journal, launchd, or the terminal) |
| `log_file` set | Append to that path |

`log_level` is applied at start (`debug`, `info`, `warning`, `error`, `fatal`). Startup logs listen address and ports. Shutdown logs `simple-ldapd stopped`.

## What to watch

- Process / unit running
- Listen sockets 389/636 (or 3389 in the lab)
- Bind failures in client applications (`invalidCredentials`, `confidentialityRequired`, `busy`)
- Disk space and permissions on `ldif_file` and `log_file`
- Latency under load (session threads share one in-memory tree)

## Useful commands

```bash
sudo journalctl -u simple-ldapd -f
sudo journalctl -u simple-ldapd --since "1 hour ago"
ss -tlnp | grep -E '389|3389|636|6636'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b '' -s base '(objectClass=*)' vendorVersion
```

Health checks that only open TCP (for example `nc -z localhost 389`) confirm the listen socket, not LDAP correctness.
