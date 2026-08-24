# Implementation summary

- Library under `include/simple-ldapd` and `src/simple-ldapd`
- Multi-binary CMake targets: `simple-ldapd`, `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`
- RFC 4511 BER codec for bind, search, and unbind
- Simple bind (anonymous, root DN + `root_password`, entry `userPassword`)
- Search with equality / and / or / not / present filters on the memory backend
- Optional LDIF seed via `ldif_file`; `userPassword` hidden unless bound as root DN
- Working `ldapsearch` client; write-tool binaries still stubs
- SASL/TLS/rate-limiter types reserved for later milestones
- Schema packs: core, cosine, inetOrgPerson, nis, ad-compat
