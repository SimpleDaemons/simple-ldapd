# Technical debt

- Search filters are a subset (no substrings, extensible match, or approximate)
- Accept loop is single-threaded (one connection at a time)
- Writes are allowed only for the configured root DN (no per-entry ACLs)
- GSSAPI lab tickets are HMAC-SHA256, not MIT Kerberos / RFC 4120
- SASL EXTERNAL trusts the authzid on TLS without verifying a client certificate
- Client tool names (`ldapsearch`, …) collide with OpenLDAP if installed on the same prefix
- jsoncpp is optional and unused by the current config parser
- `ldappasswd` is still a stub
