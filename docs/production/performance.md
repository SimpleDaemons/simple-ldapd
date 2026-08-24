# Production performance

v0.15.0 serves overlapping LDAP clients on one host and applies `acl` search/write rules. It is still sized for labs and small SSO bind/search loads, not a multi-tenant directory farm.

## Accept loop

`LdapDaemon::acceptLoop` polls the LDAP and LDAPS listeners and starts a session thread per connection. Directory maps are mutex-protected; SQLite writes run in transactions. LDIF persist (when that backend is used) is serialized so concurrent writes do not interleave the file.

A stuck client no longer blocks accept of others. CPU still grows with tree size; SQLite avoids rewriting the whole directory on each write.

## What is cheap vs expensive

| Path | Cost |
|------|------|
| Simple bind by exact DN | One lookup |
| Bind by uid / sAMAccountName | Subtree search of `base_dn` |
| Search with a selective filter | Scan of in-scope entries (memory or loaded from SQLite) |
| Substring / and-or-not | Same scan; match is in-process string compare |
| Write + SQLite | Transaction commit to `sqlite_file` |
| Write + LDIF persist | Rewrite the entire `ldif_file` |

SQLite is the production store. Very large DIT sizes are still limited by RAM because search loads matching entries into process memory. The LDIF backend additionally rewrites the whole file on each write.

## TLS

LDAPS handshakes before the first LDAP PDU. StartTLS handshakes after a successful extended response. OpenSSL is used when `ENABLE_SSL` is on.

## Tests

`test_ldap_performance` in `ctest` is a coarse sanity check, not a capacity plan. Measure bind and search latency on your hardware with the real schema and LDIF before promising SLAs.
