# Schema files

OpenLDAP-style `attributetype` / `objectclass` definitions (NAME, SUP, SYNTAX, EQUALITY, SINGLE-VALUE, MUST, MAY). The daemon loads every `*.schema` file in `schema_dir` and enforces the result on add, modify, and modrdn.

| File | Purpose |
|------|---------|
| `core.schema` | RFC 4512/4519 core |
| `cosine.schema` | RFC 4524 COSINE (`uid`, `mail`, …) |
| `inetorgperson.schema` | RFC 2798 inetOrgPerson |
| `nis.schema` | RFC 2307 posixAccount / posixGroup / shadowAccount |
| `ad-compat.schema` | AD-like names (`sAMAccountName`, `memberOf`, auxiliary `user`) |

`user` is auxiliary so an `inetOrgPerson` entry can also carry AD bind attributes. This is not a domain controller.

Drop additional `*.schema` files in this directory (or `schema_dir` in config) to extend the registry.
