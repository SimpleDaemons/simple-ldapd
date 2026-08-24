# Technical debt

- Schema parser does not handle multi-line OpenLDAP schema and is not enforced on writes
- Search filters are a subset (no substrings, extensible match, or approximate)
- Accept loop is single-threaded (one connection at a time)
- Writes are allowed only for the configured root DN (no per-entry ACLs)
- SASL mechanisms advertise names but always return `authMethodNotSupported`
- Client tool names (`ldapsearch`, …) collide with OpenLDAP if installed on the same prefix
- jsoncpp is optional and unused by the current config parser
- `ldappasswd` is still a stub
