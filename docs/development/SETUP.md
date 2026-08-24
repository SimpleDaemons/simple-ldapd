# Development setup

## Clone

Standalone:

```bash
git clone https://github.com/SimpleDaemons/simple-ldapd.git
cd simple-ldapd
```

Or from the SimpleDaemons monorepo submodule at `projects/simple-ldapd`.

## Dependencies and build

Install compilers and OpenSSL from the [build guide](BUILD_GUIDE.md), then:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Lab configuration

Use `config/templates/development.conf`:

- Listen **127.0.0.1:3389** (LDAP) and **6636** (LDAPS, off by default)
- In-memory tree seeded from `config/examples/simple/example.ldif` (non-empty `ldif_file` uses the LDIF backend, so writes persist)
- Production templates use `backend = sqlite` and `sqlite_file` instead of rewriting LDIF
- Root DN `cn=admin,dc=example,dc=com` / password `secret`
- Seeded user `uid=alice,ou=People,dc=example,dc=com` / password `alice-secret`
- Lab GSSAPI keytab `config/examples/simple/lab.keytab`

Validate, then run in the foreground:

```bash
./build/simple-ldapd --test-config --config config/templates/development.conf
./build/simple-ldapd --foreground --config config/templates/development.conf
```

`--daemon` / `foreground = false` only logs that daemonize is not implemented; the process stays in the foreground. Stop with SIGINT or SIGTERM.

Production templates use ports **389** / **636** and need root (or `CAP_NET_BIND_SERVICE`) plus TLS files. See [production](../production/README.md).

## Schema path

`schema_dir` must point at the `schemas/` tree (or a copy). Relative paths are resolved from the working directory, so start the daemon from the repo root during development.

## TLS in the lab

Uncomment `tls_cert_file` / `tls_key_file` and set `enable_ldaps` or `enable_starttls` after generating a cert. The client needs `--ca-file` (or the server cert) to trust it. The high-security template also sets `require_confidentiality`.
