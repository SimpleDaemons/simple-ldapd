# Architecture (skeleton)

```
LDAP clients and CLI tools
        |
        v
 TLS / StartTLS (stub) ---- TcpListener (optional bind)
        |
        v
     Session ---- Bind / SASL stubs
        |
        v
  LDAPv3 codec (stub) ---- SchemaRegistry
        |
        v
     Backend API
      /        \
MemoryBackend  LdifBackend
```

SSO for v0.x is **LDAP simple bind**: applications present a DN (or later a mapped `sAMAccountName` / `uid`) and password. Kerberos, OIDC, and SAML are out of this daemon.
