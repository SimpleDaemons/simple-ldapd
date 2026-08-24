# Technical debt

- Search filters are a subset (no substrings, extensible match, or approximate)
- Accept loop is single-threaded (one connection at a time)
- Writes are allowed only for the configured root DN (no per-entry ACLs)
- GSSAPI is advertised but returns `authMethodNotSupported` until a ticket source exists
- SASL EXTERNAL trusts the authzid on TLS without verifying a client certificate
- Client tool names (`ldapsearch`, …) collide with OpenLDAP if installed on the same prefix
- jsoncpp is optional and unused by the current config parser
- `ldappasswd` is still a stub
