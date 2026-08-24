# User guide

OpenLDAP-style tools talk to a running simple-ldapd. Default names match OpenLDAP; use `-DLDAP_CLI_PREFIX=simple-` or a private `CMAKE_INSTALL_PREFIX` if you need both on `PATH`.

| Document | Contents |
|----------|----------|
| [CLI](cli.md) | Daemon flags and client flags |
| [Search](search.md) | Filters, scope, Root DSE, `userPassword` |
| [First steps](../getting-started/first-steps.md) | Bind names, writes, passwords, SASL, TLS |
| [Examples](../examples/README.md) | Copy-paste commands |
| [Troubleshooting](../troubleshooting/README.md) | Common failures |

The daemon is LDAPv3 only. Bind version other than 3 returns `protocolError`.
