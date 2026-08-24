# Command line interface

## simple-ldapd

```bash
simple-ldapd [OPTIONS] [COMMAND]
```

| Option | Description |
|--------|-------------|
| `--help`, `-h` | Usage |
| `--version`, `-v` | Version and platform |
| `--config`, `-c FILE` | Configuration file |
| `--foreground`, `-f` | Stay in the foreground |
| `--daemon`, `-d` | Request daemonize (not implemented in the skeleton) |
| `--test-config` | Validate configuration and exit |

| Command | Description |
|---------|-------------|
| `start` | Start the directory (default) |
| `stop` / `status` / `reload` | Reserved; not implemented in v0.1.0 |

Examples:

```bash
simple-ldapd --version
simple-ldapd --test-config -c config/templates/development.conf
simple-ldapd --foreground -c config/templates/development.conf
```

## Client tools

`ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, and `ldappasswd` accept OpenLDAP-compatible flags. Wire operations are not implemented yet; unknown work exits with status `2`.

| Flag | Meaning |
|------|---------|
| `-H URI` | `ldap://` or `ldaps://` URI |
| `-h HOST` | Host (OpenLDAP-compatible; implies `ldap://HOST`) |
| `-x` | Simple authentication |
| `-D BIND_DN` | Bind DN |
| `-w PASSWORD` | Bind password |
| `-W` | Prompt for password |
| `-b BASE_DN` | Search base |
| `-f FILE` | LDIF input file |
| `--help` | Usage |
| `--version` | Version |

```bash
ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(uid=alice)'
ldapadd -x -D cn=admin,dc=example,dc=com -w secret -f example.ldif
ldappasswd -x -D uid=alice,ou=People,dc=example,dc=com -W
```
