# Production deployment

Unit files and Docker examples live in [deployment/](../../deployment/README.md). Packaging templates: [packaging/README.md](../../packaging/README.md).

Linux packages and `cmake --install` create `/var/lib/simple-ldapd` and `/var/log/simple-ldapd` (macOS/Windows equivalents below). Copy a production template, set `root_password` and TLS files, then enable the supervisor. Packages do not start the service on install.

## Linux (systemd)

The shipped unit already matches the default prefix (`/usr`):

```
ExecStart=/usr/bin/simple-ldapd --config /etc/simple-ldapd/simple-ldapd.conf --foreground
```

It runs as `simple-ldapd`, creates `StateDirectory` / `LogsDirectory`, and grants `CAP_NET_BIND_SERVICE` so ports 389/636 work without root.

1. Install the package, or `cmake --install` and copy `deployment/systemd/simple-ldapd.service` to `/etc/systemd/system/` if you did not install the unit.
2. Copy `config/templates/production.conf` (or high-security) to `/etc/simple-ldapd/simple-ldapd.conf` if the package did not already.
3. Install TLS material under `/etc/simple-ldapd/tls/` and set `root_password`. Mode `640`, owner `root:simple-ldapd`.
4. `systemctl daemon-reload && systemctl enable --now simple-ldapd`
5. `systemctl status simple-ldapd` and `journalctl -u simple-ldapd -f`

If `CMAKE_INSTALL_PREFIX` is not `/usr`, edit `ExecStart` to the installed binary and config paths.

## macOS (launchd)

The plist starts `/usr/local/bin/simple-ldapd --config /etc/simple-ldapd/simple-ldapd.conf --foreground`. The pkg postinstall creates `/var/lib/simple-ldapd` and `/var/log/simple-ldapd`.

Copy `deployment/launchd/com.simpledaemons.simple-ldapd.plist` to `/Library/LaunchDaemons/` if you installed from source, `chown root:wheel`, then `launchctl load`. Ports 389/636 need a privileged process on macOS.

## Windows

`deployment/windows/simple-ldapd.service` (run as Administrator) creates `%PROGRAMDATA%\simple-ldapd\` (data, logs, tls) and registers:

```
simple-ldapd.exe --config %PROGRAMDATA%\simple-ldapd\simple-ldapd.conf --foreground
```

Point that config at Windows paths for TLS, `schema_dir`, and `sqlite_file`.

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

Linux: `deployment/logrotate.d/simple-ldapd` → `/etc/logrotate.d/` (installed by the package). If `log_file` is empty, logs go to stderr and the supervisor (journald).
