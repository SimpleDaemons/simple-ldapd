# Technical debt

- Schema parser does not handle multi-line OpenLDAP schema
- LDIF export is a stub; import ignores changetype records and does not persist writes
- Search filters are a subset (no substrings, extensible match, or approximate)
- Accept loop is single-threaded (one connection at a time)
- SASL mechanisms advertise names but always return `authMethodNotSupported`
- TLS context only checks that cert/key files exist
- Client tool names (`ldapsearch`, …) collide with OpenLDAP if installed on the same prefix
- jsoncpp is optional and unused by the current config parser
