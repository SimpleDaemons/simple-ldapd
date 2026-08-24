# Implementation summary

- Library under `include/simple-ldapd` and `src/simple-ldapd`
- Multi-binary CMake targets: `simple-ldapd`, `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`
- RFC 4511 BER codec for bind, search, unbind, add, modify, delete, and modrdn
- Simple bind (anonymous, root DN + `root_password`, entry `userPassword`)
- Search with equality / and / or / not / present filters on the memory backend
- Optional LDIF seed via `ldif_file`; writes persist back to that file
- `userPassword` hidden unless bound as root DN; writes require a root DN bind
- Working `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete` (including `ldaps://` and `-Z`)
- Schema files for core, cosine, inetOrgPerson, posix, and AD-compat names; writes are checked against MUST/MAY/SYNTAX
- LDAPS and StartTLS via OpenSSL; `require_confidentiality` blocks cleartext password binds
- Schema packs: core, cosine, inetOrgPerson, nis, ad-compat (enforced on writes)
- SASL/rate-limiter types reserved for later milestones
