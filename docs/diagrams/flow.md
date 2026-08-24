# Protocol flows

## Session loop

```mermaid
flowchart TD
  Start([accept TCP]) --> TLS{LDAPS?}
  TLS -->|yes| Handshake[TLS handshake]
  TLS -->|no| Read
  Handshake --> Read[Read LDAP PDU]
  Read --> Decode{Decode BER}
  Decode -->|fail| Close([close])
  Decode -->|Unbind| Close
  Decode -->|Search| Search[Search entries then SearchResultDone]
  Decode -->|Bind| Bind[BindResponse]
  Decode -->|Add Modify Delete ModDN| Write[Result; root DN or ACL write]
  Decode -->|Compare| Cmp[CompareTrue or CompareFalse]
  Decode -->|StartTLS| STLS[ExtendedResponse then handshake]
  Decode -->|Password Modify| Pw[RFC 3062]
  Decode -->|Who Am I| Who[authzid dn: or empty]
  Decode -->|other| Err[ProtocolError or UnwillingToPerform]
  Search --> Read
  Bind --> Read
  Write --> Read
  Cmp --> Read
  STLS --> Read
  Pw --> Read
  Who --> Read
  Err --> Read
```

The accept loop starts a session thread per connection and keeps polling both listeners. `stop()` sets `running_` false and joins workers.

## Simple bind

```mermaid
sequenceDiagram
  participant C as Client
  participant S as Session
  participant A as SimpleBindAuthenticator
  participant B as Backend

  C->>S: BindRequest v3 DN password
  alt require_confidentiality and not TLS
    S-->>C: confidentialityRequired
  else empty DN
    S-->>C: success anonymous
  else resolve DN or uid or sAMAccountName
    S->>A: bind(name, password)
    A->>B: lookup or search
    A-->>S: success or invalidCredentials
    S-->>C: BindResponse
  end
```

Anonymous bind is an empty DN and empty password. A non-empty password with an empty DN is `invalidCredentials`. After a successful named bind, the session stores the **canonical** entry DN so self-service `ldappasswd` works even if the client bound as `uid=alice,dc=example,dc=com`.

## Search

```mermaid
sequenceDiagram
  participant C as Client
  participant S as Session
  participant B as Backend

  C->>S: SearchRequest base scope filter
  alt empty base and not one-level
    S->>S: maybe Root DSE
  end
  S->>B: search
  loop each match
    S-->>C: SearchResultEntry
  end
  S-->>C: SearchResultDone
```

`userPassword` is omitted unless the session is bound as the root DN.

## Password modify

```mermaid
sequenceDiagram
  participant C as ldappasswd
  participant S as Session
  participant B as Backend

  C->>S: Bind
  S-->>C: success
  C->>S: ExtendedRequest 1.3.6.1.4.1.4203.1.11.1
  alt bound as target, root DN, or ACL write
    S->>B: replace userPassword as {SSHA}
    S-->>C: success
  else other identity
    S-->>C: insufficientAccessRights
  end
```

`root_password` is config-only and cannot be changed with this operation.
