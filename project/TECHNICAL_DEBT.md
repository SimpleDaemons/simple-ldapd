# Technical debt

- BER codec and session accept loop not present; listener bind is optional
- Schema parser does not handle multi-line OpenLDAP schema
- LDIF export is a stub; import ignores changetype records
- SASL mechanisms advertise names but always return `authMethodNotSupported`
- TLS context only checks that cert/key files exist
- Client tool names (`ldapsearch`, …) collide with OpenLDAP if installed on the same prefix
- jsoncpp is optional and unused by the current config parser
