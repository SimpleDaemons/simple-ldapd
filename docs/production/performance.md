# Production performance

v0.9.0 is sized for labs and small SSO bind/search loads, not a multi-tenant directory farm.

## Accept loop

`LdapDaemon::acceptLoop` accepts one TCP connection, then `Session::serve` until unbind or disconnect, then accepts the next. LDAP and LDAPS share that loop (short timeouts when both ports are enabled). Concurrent clients queue in the kernel listen backlog; they are not handled in parallel.

If you need overlapping binds from many application servers, run more than one instance (separate ports or hosts) or wait for a concurrent accept milestone.

## What is cheap vs expensive

| Path | Cost |
|------|------|
| Simple bind by exact DN | One lookup |
| Bind by uid / sAMAccountName | Subtree search of `base_dn` |
| Search with a selective filter | Scan of in-scope entries in memory |
| Substring / and-or-not | Same scan; match is in-process string compare |
| Write + LDIF persist | Rewrite the entire `ldif_file` |

The tree is in memory. Very large DIT sizes are limited by RAM and by persist rewriting the whole LDIF.

## TLS

LDAPS handshakes before the first LDAP PDU. StartTLS handshakes after a successful extended response. OpenSSL is used when `ENABLE_SSL` is on.

## Tests

`test_ldap_performance` in `ctest` is a coarse sanity check, not a capacity plan. Measure bind and search latency on your hardware with the real schema and LDIF before promising SLAs.
