# Production security

This is a bind-centric directory, not an AD DC. Treat `root_dn` as superuser.

## Transport

- Enable LDAPS (`enable_ldaps`) and/or StartTLS (`enable_starttls`).
- Set `tls_cert_file` and `tls_key_file`. Restrict key permissions (`600`).
- Set `require_confidentiality = true` so simple binds that send a password on cleartext get `confidentialityRequired`.
- Clients need `--ca-file` (or a store that already trusts the issuer).

## Secrets

- `root_password` in the config file is the directory manager password. Mode `640`, not world-readable.
- Entry `userPassword` values are compared as given, or with a `{CLEARTEXT}` prefix stripped. There is no `{SSHA}` / `{CRYPT}` hashing yet.
- `ldappasswd` cannot change `root_password`.
- Do not commit live LDIF after password changes.

## Authentication surface

| Bind | Production note |
|------|-----------------|
| Anonymous | Empty DN + empty password always binds. Search is allowed with no `acl` lines; `acl = users search *` denies anonymous reads. |
| Simple | Use TLS. Bind DN resolution accepts uid / sAMAccountName. |
| SASL PLAIN / DIGEST-MD5 | Same identities as simple bind. PLAIN still needs TLS. |
| SASL EXTERNAL | Trusts the authzid DN; **does not verify a client certificate**. |
| SASL GSSAPI | Lab HMAC tickets only. Not MIT Kerberos. |

## Authorization

Writes require `root_dn` or an `acl` write rule covering the target DN. Password Modify allows self-change, root-set, or write ACL. Schema is enforced on add/modify/modrdn, not on search. The high-security template uses `acl = users search *` so anonymous cannot read the tree.

## Operational hygiene

- Dedicated service account; do not run as root except to bind 389/636 (or use capabilities).
- Keep `schema_dir` read-only to the service user if you do not expect runtime schema edits.

Diagram: [security](../diagrams/security.md).
