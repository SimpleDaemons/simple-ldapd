# Troubleshooting

## Daemon will not start

1. `./build/simple-ldapd --test-config -c path/to.conf` and read stderr.
2. Confirm `schema_dir` exists relative to the **current working directory**.
3. If `enable_ldaps` or `enable_starttls` is true, `tls_cert_file` and `tls_key_file` must exist.
4. Port in use: development **3389**, production **389**. `ss -tlnp | grep 3389`.
5. Permission to bind 389/636 requires root or `CAP_NET_BIND_SERVICE`.

`stop` / `status` / `reload` always fail with exit 2; they are not implemented.

## Client cannot connect

- Lab URI must include the port: `-H ldap://127.0.0.1:3389` (default client port is 389).
- Only one connection is served at a time. A stuck client (or a tool waiting on stdin) blocks everyone else. Unbind or kill the holder.
- LDAPS vs LDAP: `ldaps://` uses the LDAPS listener; `-Z` is StartTLS on the LDAP port.

## invalidCredentials

- Alice's seeded DN is `uid=alice,ou=People,dc=example,dc=com`, password `alice-secret`.
- `uid=alice,dc=example,dc=com` and bare `alice` should resolve; rebuild if you are on a tag older than v0.8.1.
- `{CLEARTEXT}` prefix is 11 characters if you store hashed-looking values that way.
- Anonymous bind must use an empty password.

## confidentialityRequired

`require_confidentiality` is on and the bind sent a password on a cleartext connection. Use `ldaps://`, `-Z`, or turn the setting off in the lab.

## insufficientAccessRights on writes

Add/modify/delete/modrdn require the configured `root_dn`. Ordinary users can only change their own password via RFC 3062 / `ldappasswd`.

## Schema errors on add/modify

The entry must satisfy MUST/MAY/SYNTAX from `schema_dir`. `user` is auxiliary (AD-compat); `inetOrgPerson` still needs `sn` and `cn`. See [schemas/README.md](../../../schemas/README.md).

## userPassword missing from search

Expected unless you bind as the root DN.

## ldappasswd does not change the admin password

`root_password` is config-only. Password Modify refuses the root DN as a target.

## GSSAPI bind fails

- `gssapi_keytab` must be the **text** lab keytab, not a MIT `krb5` binary keytab.
- Client: `--keytab` or `SIMPLE_LDAPD_KTNAME`.
- This is not interoperable with `kinit` / real tickets.

## Seed file password changed

If `ldif_file` points at `config/examples/simple/example.ldif`, a successful `ldappasswd` rewrites that file. Restore `alice-secret` from git and avoid committing the mutated seed.

## TLS handshake fails

Pass `--ca-file` to the client. Confirm `enable_ldaps` / `enable_starttls` match the URI (`ldaps://` vs `-Z`). Development LDAPS port is **6636**.

## Verbose logging

`log_level` in the config file is not applied yet. The process logs at info (and above) to stderr or `log_file`.

## Build / OpenSSL

See [build guide](../../development/BUILD_GUIDE.md).
