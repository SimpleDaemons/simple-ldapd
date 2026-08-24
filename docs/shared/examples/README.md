# Examples

Lab host: `127.0.0.1:3389`, template `config/templates/development.conf`. Seeded alice: `uid=alice,ou=People,dc=example,dc=com` / `alice-secret`. Root: `cn=admin,dc=example,dc=com` / `secret`.

## Search

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(cn=Ali*)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com \
  '(&(objectClass=inetOrgPerson)(uid=alice))'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b '' -s base '(objectClass=*)'
```

## SASL

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y PLAIN -U alice -w alice-secret \
  -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y DIGEST-MD5 -U alice -w alice-secret \
  -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Y GSSAPI -U alice \
  --keytab config/examples/simple/lab.keytab \
  -b dc=example,dc=com '(uid=alice)'
```

## Writes (root DN or `acl` write)

```bash
./build/ldapadd -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -f change.ldif
./build/ldapmodify -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret -f change.ldif
./build/ldapdelete -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret \
  'uid=bob,ou=People,dc=example,dc=com'
```

## Passwords

```bash
./build/ldappasswd -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret \
  -s alice-new uid=alice,ou=People,dc=example,dc=com
./build/ldappasswd -H ldap://127.0.0.1:3389 -x \
  -D uid=alice,ou=People,dc=example,dc=com -w alice-new -s alice-newer
```

## TLS (after certs and `enable_ldaps` / `enable_starttls`)

```bash
./build/ldapsearch -H ldaps://127.0.0.1:6636 --ca-file tls/ca.crt -x \
  -D cn=admin,dc=example,dc=com -w secret -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -Z --ca-file tls/ca.crt -x \
  -D cn=admin,dc=example,dc=com -w secret -b dc=example,dc=com '(uid=alice)'
```

More narrative: [first steps](../getting-started/first-steps.md), [CLI](../user-guide/cli.md), [search](../user-guide/search.md).
