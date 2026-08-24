# Production performance

v0.10.0 serves overlapping LDAP clients on one host. It is still sized for labs and small SSO bind/search loads, not a multi-tenant directory farm.

## Accept loop

`LdapDaemon::acceptLoop` polls the LDAP and LDAPS listeners and starts a session thread per connection. The in-memory tree is mutex-protected; LDIF persist is serialized so concurrent writes do not interleave the file.

A stuck client no longer blocks accept of others. CPU and file-rewrite cost still grow with tree size and write rate.

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
