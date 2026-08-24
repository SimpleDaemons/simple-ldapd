# Production operations

## Start and stop

The process stays in the foreground. Supervisors send SIGINT/SIGTERM; `LdapDaemon::stop` joins the accept thread.

```bash
# Linux
sudo systemctl start simple-ldapd
sudo systemctl stop simple-ldapd
sudo systemctl status simple-ldapd
sudo journalctl -u simple-ldapd -f

# Config check without listening
sudo simple-ldapd --test-config -c /etc/simple-ldapd/simple-ldapd.conf
```

`simple-ldapd stop|status|reload` prints that the command is not implemented and exits 2. There is no pidfile.

## Health

There is no HTTP health endpoint. Check:

1. Process / unit status
2. TCP listen (`ss -tlnp | grep -E '389|636'`)
3. LDAP search:

```bash
ldapsearch -H ldaps://127.0.0.1:636 --ca-file /etc/simple-ldapd/tls/ca.crt -x \
  -b '' -s base '(objectClass=*)' namingContexts
```

## Backup

The directory is the LDIF file when `backend = ldif`:

```bash
sudo systemctl stop simple-ldapd
sudo cp -a /var/lib/simple-ldapd/directory.ldif /var/backups/directory-$(date +%Y%m%d).ldif
sudo tar -czf /var/backups/simple-ldapd-config-$(date +%Y%m%d).tar.gz /etc/simple-ldapd/
sudo systemctl start simple-ldapd
```

Stopping first avoids copying a file mid-`persist()`. A daemon with no `ldif_file` uses the memory backend and loses the tree on restart.

## Restore

Stop the service, restore `directory.ldif` and config, start, then `--test-config` and a bind/search.

## Updates

1. Stop the unit
2. Keep the previous binary and config
3. Install the new binary
4. `--test-config`
5. Start and search the Root DSE `vendorVersion`

## Logs

`log_file` appends timestamped lines. Unset `log_file` to use stderr (journald under systemd). `log_level` is stored from config but not applied to the logger yet. There are no metrics or tracing exporters. See [observability](../shared/observability/README.md).
