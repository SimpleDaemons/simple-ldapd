# Implementation summary

- Library under `include/simple-ldapd` and `src/simple-ldapd`
- Multi-binary CMake targets: `simple-ldapd`, `ldapsearch`, `ldapadd`, `ldapmodify`, `ldapdelete`, `ldappasswd`, `ldapcompare`, `ldapwhoami`
- RFC 4511 BER codec for bind, search, unbind, add, modify, delete, and modrdn
- Simple bind (anonymous, root DN + `root_password`, entry `userPassword`)
- Search with equality / and / or / not / present / substring filters on the memory backend
- Optional LDIF seed via `ldif_file`; SQLite (`backend = sqlite`) persists each write; LDIF persist still rewrites the whole file
- `userPassword` hidden unless bound as root DN; writes require `root_dn` or an `acl` write rule
- Working `ldapsearch` / `ldapadd` / `ldapmodify` / `ldapdelete` / `ldappasswd` / `ldapcompare` / `ldapwhoami` (including `ldaps://` and `-Z`)
- Schema files for core, cosine, inetOrgPerson, posix, and AD-compat names; writes are checked against MUST/MAY/SYNTAX
- LDAPS and StartTLS via OpenSSL; `require_confidentiality` blocks cleartext password binds
- Schema packs: core, cosine, inetOrgPerson, nis, ad-compat (enforced on writes)
- SASL PLAIN, DIGEST-MD5, EXTERNAL, and GSSAPI lab tickets from `gssapi_keytab`
- RFC 3062 password modify: self-change, root-set, or `acl` write on the target
- Concurrent sessions: one thread per accepted LDAP or LDAPS connection
- Access control: repeatable `acl` lines; empty list keeps anonymous search and root-only writes
- Password storage: `{SSHA}` on write; `{CLEARTEXT}` and unprefixed still bind; `userAccountControl` bit `0x0002` disables bind
- Compare, Who Am I, RFC 2696 paged results, search `typesOnly` and time limit
- SQLite directory backend (`sqlite_file`, WAL); optional LDIF seed when the database is empty
