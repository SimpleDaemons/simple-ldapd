# First steps

Assumes the [quick start](quick-start.md) daemon is running with `config/templates/development.conf`.

## Root DSE

An empty search base is the Root DSE (`namingContexts`, `supportedSASLMechanisms`, `supportedExtension`):

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b '' -s base '(objectClass=*)'
```

## Bind names

Simple bind accepts:

- The entry DN: `uid=alice,ou=People,dc=example,dc=com`
- A DN whose RDN value matches uid / sAMAccountName even if the parent is wrong: `uid=alice,dc=example,dc=com`
- A bare uid or sAMAccountName: `alice` (via `-D alice` or SASL `-U alice`)
- The configured root DN and `root_password`

Anonymous bind: `-x` with no `-D` / `-w`.

## Filters

Equality, present, substring, and/or/not. See [search](../user-guide/search.md).

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(cn=Ali*)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(&(objectClass=inetOrgPerson)(uid=alice))'
```

## Writes

Add, modify, delete, and modrdn require a **root DN** bind.

```bash
./build/ldapadd -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -f change.ldif
./build/ldapmodify -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -f change.ldif
./build/ldapdelete -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret \
  'uid=bob,ou=People,dc=example,dc=com'
```

Schema MUST/MAY/SYNTAX is enforced on those writes. The development template sets `ldif_file`, so successful writes persist back to that seed file.

## Passwords

RFC 3062 via `ldappasswd`. `-s` sets the new password without prompting. Success prints `Password Changed`.

```bash
# root sets alice
./build/ldappasswd -H ldap://127.0.0.1:3389 -x \
  -D cn=admin,dc=example,dc=com -w secret \
  -s alice-new uid=alice,ou=People,dc=example,dc=com

# self-change
./build/ldappasswd -H ldap://127.0.0.1:3389 -x \
  -D uid=alice,ou=People,dc=example,dc=com -w alice-new \
  -s alice-newer
```

Do not commit a mutated `example.ldif` after a live `ldappasswd` against the seed file.

## SASL (lab)

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y PLAIN -U alice -w alice-secret \
  -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y DIGEST-MD5 -U alice -w alice-secret \
  -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y GSSAPI -U alice \
  --keytab config/examples/simple/lab.keytab \
  -b dc=example,dc=com '(uid=alice)'
```

GSSAPI lab tickets are HMAC, not MIT Kerberos.

## TLS

Enable `enable_ldaps` / `enable_starttls` and point `tls_cert_file` / `tls_key_file` at a cert. Clients:

```bash
./build/ldapsearch -H ldaps://127.0.0.1:6636 --ca-file tls/ca.crt -x \
  -D cn=admin,dc=example,dc=com -w secret -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Z --ca-file tls/ca.crt -x \
  -D cn=admin,dc=example,dc=com -w secret -b dc=example,dc=com '(uid=alice)'
```
