# Progress report

**v1.0.0** is the production contract cut on top of milestones 1–15. See [VERSIONING.md](../VERSIONING.md) and the 1.0 section in [ROADMAP.md](../ROADMAP.md).

What works today: concurrent LDAPv3 sessions; anonymous/simple/SASL bind; search (equality/present/substring/and-or-not); writes; Compare; Who Am I; paged results; LDAPS/StartTLS; ACLs; `{SSHA}` passwords; SQLite/LDIF; `log_level`; `bind_rate_limit`; `max_pdu_size` / `max_sessions` / `idle_timeout`; OpenLDAP-style CLI; packaging units; GitHub Actions CI + `ctest`.

What does not work (by design for 1.0): MIT Kerberos / in-tree KDC; `--daemon` fork; `stop` / `reload` CLI (use the OS supervisor); replication / referrals / overlays / AD DC.

Note: `test_ldap_bind_search` can flake under full-suite parallel load; it passes when run alone.
