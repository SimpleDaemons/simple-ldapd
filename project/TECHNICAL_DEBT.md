# Technical debt

Tracked as [roadmap](../ROADMAP.md) milestones where they are product work.

- Client tool names (`ldapsearch`, …) still collide with OpenLDAP unless `-DLDAP_CLI_PREFIX=simple-` (or a private `CMAKE_INSTALL_PREFIX`) is used
- Search filters still omit ordering, approximate, and extensible match — later
- `memberOf` is a static attribute, not maintained from group membership — later
- GSSAPI lab tickets are HMAC-SHA256, not MIT Kerberos / RFC 4120 — later / out of tree
- LDIF persist still rewrites the whole file when `backend = ldif` (production templates use sqlite)
- jsoncpp is optional and unused by the current config parser
- `root_password` lives in config and cannot be changed with `ldappasswd` (intentional)
- `stop` / `status` / `reload` and `--daemon` fork are not implemented (run under a supervisor)
