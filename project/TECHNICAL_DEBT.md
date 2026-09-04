# Technical debt

Tracked intentionally; none of these block the 1.0.0 contract.

- `test_ldap_bind_search` can flake under full `ctest` parallel load; passes alone.
- DIGEST-MD5 needs recoverable passwords; `{SSHA}` is salted SHA-1 (not modern KDFs).
- GSSAPI uses HMAC lab tickets, not MIT / RFC 4120.
- SQLite search still loads matching entries in-process.
- `memberOf` is a static attribute (not computed).
- `--daemon` does not fork; use the OS supervisor.
