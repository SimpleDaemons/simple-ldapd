# Production deployment

Unit files and Docker examples live in [deployment/](../../deployment/README.md). Packaging templates: [packaging/README.md](../../packaging/README.md).

## Linux (systemd)

1. Install the binary and copy `deployment/systemd/simple-ldapd.service` to `/etc/systemd/system/`.
2. Create a system user (`simple-ldapd`) and directories for config, data, and logs.
3. Install schemas, TLS material, and a SQLite (or empty) directory file with correct ownership.
4. `systemctl daemon-reload && systemctl enable --now simple-ldapd`
5. `systemctl status simple-ldapd` and `journalctl -u simple-ldapd -f`

Edit the unit's `ExecStart` so it passes `--foreground --config /etc/simple-ldapd/simple-ldapd.conf`.

## macOS (launchd)

Copy `deployment/launchd/com.simpledaemons.simple-ldapd.plist` to `/Library/LaunchDaemons/`, `chown root:wheel`, then `launchctl load`. Confirm the plist `ProgramArguments` include `--foreground` and an absolute config path.

## Windows

`deployment/windows/simple-ldapd.service.bat` installs, starts, stops, and removes a service. Run it as Administrator. Point the service at a config with Windows paths for TLS and the SQLite file.

## Docker

Follow [deployment/examples/docker/README.md](../../deployment/examples/docker/README.md). Map 389/636, mount config and data, and use a process that stays in the foreground.

## Firewall

Allow only what clients need:

```bash
# LDAP and/or LDAPS
sudo ufw allow 389/tcp
sudo ufw allow 636/tcp
```

Prefer LDAPS-only exposure when `enable_starttls` is unused.

## Log rotation

Linux: `deployment/logrotate.d/simple-ldapd` → `/etc/logrotate.d/`. If `log_file` is empty, logs go to stderr and the supervisor (journald).
