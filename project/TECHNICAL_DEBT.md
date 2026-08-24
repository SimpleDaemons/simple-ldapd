# Technical debt

Tracked as [roadmap](../ROADMAP.md) milestones where they are product work.

- Writes are allowed only for the configured root DN (no per-entry ACLs) — milestone 11
- `userPassword` is compared as cleartext (`{CLEARTEXT}` prefix only) — milestone 12
- `userAccountControl` is schema-only (not enforced on bind) — milestone 12
- No Compare, Who Am I, or paged results — milestone 13
- Search `time_limit` is decoded and ignored — milestone 13
- LDIF persist rewrites the whole file — milestone 14
- `log_level` is parsed and not applied — milestone 15
- `RateLimiter` is a stub — milestone 15
- SASL EXTERNAL trusts the authzid on TLS without verifying a client certificate — milestone 15
- Client tool names (`ldapsearch`, …) collide with OpenLDAP if installed on the same prefix — milestone 15
- Search filters still omit ordering, approximate, and extensible match — later
- `memberOf` is a static attribute, not maintained from group membership — later
- GSSAPI lab tickets are HMAC-SHA256, not MIT Kerberos / RFC 4120 — later / out of tree
- jsoncpp is optional and unused by the current config parser
- `root_password` lives in config and cannot be changed with `ldappasswd` (intentional)
- `stop` / `status` / `reload` and `--daemon` fork are not implemented (run under a supervisor)
