# Quick start

Run a lab directory on **3389** without root, using the seeded example tree.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build
```

See [installation](installation.md) and the [build guide](../../development/BUILD_GUIDE.md) for packages and dependencies.

## Start

From the repository root:

```bash
./build/simple-ldapd --test-config --config config/templates/development.conf
./build/simple-ldapd --foreground --config config/templates/development.conf
```

The development template listens on `127.0.0.1:3389`, seeds `config/examples/simple/example.ldif`, and uses root DN `cn=admin,dc=example,dc=com` / password `secret`.

## Search

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(uid=alice)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(cn=Ali*)'
```

Seeded alice is `uid=alice,ou=People,dc=example,dc=com` with password `alice-secret`. Bind as that DN, as `uid=alice,dc=example,dc=com`, or as `alice`.

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -x \
  -D uid=alice,ou=People,dc=example,dc=com -w alice-secret \
  -b dc=example,dc=com '(uid=alice)'
```

## Next

- [First steps](first-steps.md) — writes, passwords, SASL, TLS
- [CLI](../user-guide/cli.md)
- [Configuration](../configuration/README.md)
