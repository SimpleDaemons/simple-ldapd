# Search

Search is LDAPv3 `SearchRequest` with RFC 4511 filters. The memory, LDIF, and SQLite backends share the same match logic.

## Scope

| `-s` | Meaning |
|------|---------|
| `base` | The base object only |
| `one` | Immediate children |
| `sub` | Base and descendants (default) |

Empty base DN:

- `base` — Root DSE only
- `sub` — Root DSE plus a subtree search of the directory
- `one` — one-level search of the empty base (no synthetic Root DSE)

If the base DN is non-empty and missing, the server returns `noSuchObject`.

## Root DSE attributes

Anonymous-readable:

- `namingContexts` — `base_dn`
- `supportedLDAPVersion` — `3`
- `vendorName` / `vendorVersion`
- `supportedExtension` — Password Modify (`1.3.6.1.4.1.4203.1.11.1`), Who Am I (`1.3.6.1.4.1.4203.1.11.3`); StartTLS when enabled (`1.3.6.1.4.1.1466.20037`)
- `supportedControl` — paged results (`1.2.840.113556.1.4.319`)
- `supportedSASLMechanisms` — advertised SASL names

## Filters

String filters must be wrapped in parentheses. `ldapsearch` wraps a bare token as `(token)`.

| Form | Type |
|------|------|
| `(attr=value)` | Equality (case-insensitive) |
| `(attr=*)` | Present |
| `(attr=Ali*)` | Substring initial |
| `(attr=*Exam*)` | Substring any |
| `(attr=*example)` | Substring final |
| `(attr=A*e)` | Initial and final |
| `(attr=Ali*Ex*ple)` | Initial, any, final |
| `(&(f1)(f2))` | And |
| `(\|(f1)(f2))` | Or |
| `(!(f))` | Not |

`(attr=*)` is present, not a substring of everything. Other `*` in the assertion value become RFC 4511 substring components (`initial` / `any` / `final`), encoded as BER tag `0xA4`.

Not implemented: extensible match, approximate match (`~=`), greater/less ordering.

## Attributes

- Default (no extra arguments): all user attributes except `userPassword` unless bound as root DN
- Named attributes: only those types
- `*` — all user attributes (same `userPassword` rule)
- `1.1` — no attributes (DN only)
- `-A` / `typesOnly` — attribute names with empty values

## Size, time, and pages

If the client sends a size limit greater than zero, extra matches produce `sizeLimitExceeded` after that many entries. A time limit greater than zero is wall-clock seconds for that search (`timeLimitExceeded`). RFC 2696 paged results (`-E pr=N`) return a cookie on `SearchResultDone` until the last page.

## Examples

```bash
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(objectClass=*)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b dc=example,dc=com '(cn=Ali*)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -b ou=People,dc=example,dc=com -s one '(uid=*)'
./build/ldapsearch -H ldap://127.0.0.1:3389 -x -D cn=admin,dc=example,dc=com -w secret \
  -b uid=alice,ou=People,dc=example,dc=com -s base '(objectClass=*)' userPassword
```
