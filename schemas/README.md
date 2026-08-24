# Schema files

These files are **name/OID placeholders** for the skeleton. A later milestone will parse full OpenLDAP-style schema (SUP, MUST/MAY, SYNTAX, EQUALITY).

| File | Purpose |
|------|---------|
| `core.schema` | RFC 4512/4519 core |
| `cosine.schema` | RFC 4524 COSINE |
| `inetorgperson.schema` | RFC 2798 inetOrgPerson |
| `nis.schema` | RFC 2307 posixAccount / posixGroup |
| `ad-compat.schema` | AD-like names for bind/search compatibility |

Drop additional `*.schema` files in this directory (or `schema_dir` in config) to extend the registry.
