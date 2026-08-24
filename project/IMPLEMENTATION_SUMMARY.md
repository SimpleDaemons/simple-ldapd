# Implementation summary

- Library under `include/simple-ldapd` and `src/simple-ldapd`
- Multi-binary CMake targets: `simple-ldapd`, `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`
- Backend interface with `MemoryBackend` and `LdifBackend`
- SASL/TLS/rate-limiter types reserved for later milestones
- Schema packs: core, cosine, inetOrgPerson, nis, ad-compat
