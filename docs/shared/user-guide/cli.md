# Command line interface

## simple-ldapd

```bash
simple-ldapd [OPTIONS] [COMMAND]
```

| Option | Description |
|--------|-------------|
| `--help`, `-h` | Usage |
| `--version`, `-v` | Version, description, and platform |
| `--config`, `-c FILE` | Configuration file (key=value) |
| `--foreground`, `-f` | Stay in the foreground (sets `foreground = true`) |
| `--daemon`, `-d` | Sets `foreground = false`; **does not fork** (warning is logged) |
| `--test-config` | Validate configuration and exit |

| Command | Behavior |
|---------|----------|
| `start` | Start the directory (default) |
| `test` | Same as `--test-config` |
| `stop` / `status` / `reload` | Not implemented (exit status 2) |

Examples:

```bash
simple-ldapd --version
simple-ldapd --test-config -c config/templates/development.conf
simple-ldapd --foreground -c config/templates/development.conf
```

SIGINT and SIGTERM stop the accept loop. There is no pidfile or `simple-ldapd stop`.

## Client tools

`ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, and `ldappasswd` share OpenLDAP-compatible flags.

| Flag | Meaning |
|------|---------|
| `-H URI` | `ldap://host:port` or `ldaps://host:port` |
| `-h HOST` | Host (implies `ldap://HOST`) |
| `-p PORT` | Port |
| `-Z` | StartTLS after connect |
| `--ca-file FILE` | Trust CA or server cert for TLS |
| `-x` | Simple authentication (default) |
| `-Y MECH` | SASL: `PLAIN`, `DIGEST-MD5`, `EXTERNAL`, `GSSAPI` |
| `-U AUTHCID` | SASL authentication identity |
| `--keytab FILE` | Lab GSSAPI keytab (else `SIMPLE_LDAPD_KTNAME`) |
| `-D BIND_DN` | Bind DN, uid, or sAMAccountName |
| `-w PASSWORD` | Bind password |
| `-W` | Prompt for bind password |
| `-b BASE_DN` | Search base |
| `-s SCOPE` | `base`, `one`, or `sub` (default `sub`; not used by `ldappasswd`) |
| `-a` | `ldapmodify`: treat records as add; `ldappasswd`: old password |
| `-f FILE` | LDIF input |
| `--help` | Usage |
| `--version` | Version |

A positional argument that does not start with `(` is wrapped as `(filter)` for search. Extra positionals after the filter are requested attributes.

Default URI is `ldap://127.0.0.1:389`. Lab daemons listen on **3389**, so pass `-H ldap://127.0.0.1:3389`.

### ldapsearch

```bash
ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(uid=alice)'
ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(cn=Ali*)' cn mail
ldapsearch -H ldap://127.0.0.1:3389 -x -b '' -s base '(objectClass=*)'
ldapsearch -H ldap://127.0.0.1:3389 -Y PLAIN -U alice -w alice-secret -b dc=example,dc=com '(uid=alice)'
```

### ldapadd / ldapmodify / ldapdelete

Writes require the root DN.

```bash
ldapadd -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -f change.ldif
ldapmodify -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -f change.ldif
ldapmodify -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -a -f add.ldif
ldapdelete -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret \
  'uid=bob,ou=People,dc=example,dc=com'
```

### ldappasswd

| Flag | Meaning |
|------|---------|
| `-s PASSWORD` | New password (non-interactive) |
| `-S` | Prompt twice for the new password |
| `-a PASSWORD` | Old password (optional when already bound as the user) |
| `[user]` | Target DN or uid; default is the bound identity |

```bash
ldappasswd -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret \
  -s alice-new uid=alice,ou=People,dc=example,dc=com
ldappasswd -H ldap://127.0.0.1:3389 -x -D uid=alice,ou=People,dc=example,dc=com -w alice-secret \
  -s alice-newer
```

Success prints `Password Changed`.
