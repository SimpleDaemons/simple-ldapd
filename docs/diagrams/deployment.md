# Deployment views

## Lab vs production ports

```mermaid
flowchart LR
  subgraph lab [Development]
    DConf[development.conf]
    D389["127.0.0.1:3389 LDAP"]
    D636["6636 LDAPS optional"]
  end

  subgraph prod [Production templates]
    PConf[production.conf or high-security.conf]
    P389["0.0.0.0:389"]
    P636["636 LDAPS"]
  end

  DConf --> D389
  DConf --> D636
  PConf --> P389
  PConf --> P636
```

Development binds unprivileged ports so the daemon can run without root. Production templates expect 389/636, TLS files under `/etc/simple-ldapd/tls/`, and an LDIF file under `/var/lib/simple-ldapd/`.

## Host layout

```mermaid
flowchart TB
  subgraph os [Host]
    Bin["/usr/sbin/simple-ldapd"]
    Conf["/etc/simple-ldapd/"]
    Data["/var/lib/simple-ldapd/directory.ldif"]
    Logs["/var/log/simple-ldapd/"]
    Unit[systemd / launchd / Windows service]
  end

  Unit --> Bin
  Bin --> Conf
  Bin --> Data
  Bin --> Logs
```

Unit files live in [deployment/](../../deployment/README.md):

- Linux: `deployment/systemd/simple-ldapd.service`
- macOS: `deployment/launchd/com.simpledaemons.simple-ldapd.plist`
- Windows: `deployment/windows/simple-ldapd.service.bat`
- Docker example: `deployment/examples/docker/`

`--daemon` does not fork. Use the OS supervisor (systemd, launchd, a Windows service, or a container entrypoint) and keep `foreground = true` or pass `--foreground`.
