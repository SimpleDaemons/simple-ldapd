# simple-ldapd Deployment

This directory contains deployment configurations and examples for simple-ldapd.

## Directory Structure

```
deployment/
├── systemd/                    # Linux systemd service files
│   └── simple-ldapd.service
├── launchd/                    # macOS launchd service files
│   └── com.simpledaemons.simple-ldapd.plist
├── logrotate.d/                # Linux log rotation configuration
│   └── simple-ldapd
├── windows/                    # Windows service management
│   └── simple-ldapd.service
└── examples/                   # Deployment examples
    └── docker/                 # Docker deployment examples
        ├── docker-compose.yml
        └── README.md
```

## Platform-Specific Deployment

### Linux (systemd)

The unit starts `/usr/bin/simple-ldapd --config /etc/simple-ldapd/simple-ldapd.conf --foreground` as user `simple-ldapd` and creates `/var/lib/simple-ldapd` and `/var/log/simple-ldapd`.

1. **Install the service file** (if you did not install the package):
   ```bash
   sudo cp deployment/systemd/simple-ldapd.service /etc/systemd/system/
   sudo systemctl daemon-reload
   ```

2. **Create user and directories:**
   ```bash
   sudo useradd --system --home-dir /var/lib/simple-ldapd --no-create-home \
     --shell /usr/sbin/nologin simple-ldapd
   sudo mkdir -p /etc/simple-ldapd/tls /var/lib/simple-ldapd /var/log/simple-ldapd
   sudo chown root:simple-ldapd /etc/simple-ldapd /etc/simple-ldapd/tls
   sudo chmod 0750 /etc/simple-ldapd /etc/simple-ldapd/tls
   sudo chown simple-ldapd:simple-ldapd /var/lib/simple-ldapd /var/log/simple-ldapd
   sudo chmod 0750 /var/lib/simple-ldapd /var/log/simple-ldapd
   ```

3. **Install config** (`640` `root:simple-ldapd`), schemas, and TLS files. Copy `config/templates/production.conf` to `/etc/simple-ldapd/simple-ldapd.conf` and set `root_password`.

4. **Enable and start the service:**
   ```bash
   sudo systemctl enable simple-ldapd
   sudo systemctl start simple-ldapd
   ```

5. **Check status:**
   ```bash
   sudo systemctl status simple-ldapd
   sudo journalctl -u simple-ldapd -f
   ```

### macOS (launchd)

1. **Install the plist file:**
   ```bash
   sudo cp deployment/launchd/com.simpledaemons.simple-ldapd.plist /Library/LaunchDaemons/
   sudo chown root:wheel /Library/LaunchDaemons/com.simpledaemons.simple-ldapd.plist
   ```

2. **Load and start the service:**
   ```bash
   sudo launchctl load /Library/LaunchDaemons/com.simpledaemons.simple-ldapd.plist
   sudo launchctl start com.simpledaemons.simple-ldapd
   ```

3. **Check status:**
   ```bash
   sudo launchctl list | grep simple-ldapd
   tail -f /var/log/simple-ldapd/simple-ldapd.out.log
   ```

### Windows

1. **Run as Administrator:**
   ```cmd
   # Install service
   deployment\windows\simple-ldapd.service install
   
   # Start service
   deployment\windows\simple-ldapd.service start
   
   # Check status
   deployment\windows\simple-ldapd.service status
   ```

2. **Service management:**
   ```cmd
   # Stop service
   deployment\windows\simple-ldapd.service stop
   
   # Restart service
   deployment\windows\simple-ldapd.service restart
   
   # Uninstall service
   deployment\windows\simple-ldapd.service uninstall
   ```

## Log Rotation (Linux)

1. **Install logrotate configuration:**
   ```bash
   sudo cp deployment/logrotate.d/simple-ldapd /etc/logrotate.d/
   ```

2. **Test logrotate configuration:**
   ```bash
   sudo logrotate -d /etc/logrotate.d/simple-ldapd
   ```

3. **Force log rotation:**
   ```bash
   sudo logrotate -f /etc/logrotate.d/simple-ldapd
   ```

## Docker Deployment

See [examples/docker/README.md](examples/docker/README.md) for detailed Docker deployment instructions.

### Quick Start

```bash
# Build and run with Docker Compose
cd deployment/examples/docker
docker-compose up -d

# Check status
docker-compose ps
docker-compose logs simple-ldapd
```

## Configuration

### Service Configuration

Each platform has specific configuration requirements:

- **Linux**: shipped unit is `/usr/bin/simple-ldapd --config /etc/simple-ldapd/simple-ldapd.conf --foreground`
- **macOS**: plist `ProgramArguments` use `/usr/local/bin/simple-ldapd` and `/etc/simple-ldapd/simple-ldapd.conf`
- **Windows**: `%PROGRAMFILES%\simple-ldapd\simple-ldapd.exe --config %PROGRAMDATA%\simple-ldapd\simple-ldapd.conf --foreground`

### Application Configuration

Place your application configuration in:
- **Linux/macOS**: `/etc/simple-ldapd/simple-ldapd.conf`
- **Windows**: `%PROGRAMDATA%\simple-ldapd\simple-ldapd.conf`

Data and logs:
- **Linux/macOS**: `/var/lib/simple-ldapd`, `/var/log/simple-ldapd`
- **Windows**: `%PROGRAMDATA%\simple-ldapd\` and `%PROGRAMDATA%\simple-ldapd\logs\`

## Security Considerations

### User and Permissions

1. **Create dedicated user:**
   ```bash
   # Linux
   sudo useradd --system --no-create-home --shell /bin/false simple-ldapd
   
   # macOS
   sudo dscl . -create /Users/_simple-ldapd UserShell /usr/bin/false
   sudo dscl . -create /Users/_simple-ldapd UniqueID 200
   sudo dscl . -create /Users/_simple-ldapd PrimaryGroupID 200
   sudo dscl . -create /Groups/_simple-ldapd GroupID 200
   ```

2. **Set proper permissions:**
   ```bash
   # Configuration files
   sudo chown root:simple-ldapd /etc/simple-ldapd/simple-ldapd.conf
   sudo chmod 640 /etc/simple-ldapd/simple-ldapd.conf
   
   # Log files
   sudo chown simple-ldapd:simple-ldapd /var/log/simple-ldapd/
   sudo chmod 755 /var/log/simple-ldapd/
   ```

### Firewall Configuration

Configure firewall rules as needed:

```bash
# Linux (ufw)
sudo ufw allow 389/tcp

# Linux (firewalld)
sudo firewall-cmd --permanent --add-port=389/tcp
sudo firewall-cmd --reload

# macOS
sudo pfctl -f /etc/pf.conf
```

## Monitoring

### Health Checks

1. **Service status:**
   ```bash
   # Linux
   sudo systemctl is-active simple-ldapd
   
   # macOS
   sudo launchctl list | grep simple-ldapd
   
   # Windows
   sc query simple-ldapd
   ```

2. **Port availability:**
   ```bash
   netstat -tlnp | grep 389
   ss -tlnp | grep 389
   ```

3. **Process monitoring:**
   ```bash
   ps aux | grep simple-ldapd
   top -p $(pgrep simple-ldapd)
   ```

### Log Monitoring

1. **Real-time logs:**
   ```bash
   # Linux
   sudo journalctl -u simple-ldapd -f
   
   # macOS
   tail -f /var/log/simple-ldapd.log
   
   # Windows
   # Use Event Viewer or PowerShell Get-WinEvent
   ```

2. **Log analysis:**
   ```bash
   # Search for errors
   sudo journalctl -u simple-ldapd --since "1 hour ago" | grep -i error
   
   # Count log entries
   sudo journalctl -u simple-ldapd --since "1 day ago" | wc -l
   ```

## Troubleshooting

### Common Issues

1. **Service won't start:**
   - Check configuration file syntax
   - Verify user permissions
   - Check port availability
   - Review service logs

2. **Permission denied:**
   - Ensure service user exists
   - Check file permissions
   - Verify directory ownership

3. **Port already in use:**
   - Check what's using the port: `netstat -tlnp | grep 389`
   - Stop conflicting service or change port

4. **Service stops unexpectedly:**
   - Check application logs
   - Verify resource limits
   - Review system logs

### Debug Mode

Run the service in debug mode for troubleshooting:

```bash
# Linux/macOS
sudo -u simple-ldapd /usr/bin/simple-ldapd --config /etc/simple-ldapd/simple-ldapd.conf --foreground

# Windows
simple-ldapd.exe --debug
```

### Log Levels

Adjust log level for more verbose output:

```bash
# Set log level in configuration
log_level = debug

# Or via environment variable
export SIMPLE_LDAPD_LOG_LEVEL=debug
```

## Backup and Recovery

### Configuration Backup

```bash
# Backup configuration
sudo tar -czf simple-ldapd-config-backup-$(date +%Y%m%d).tar.gz /etc/simple-ldapd/

# Backup logs
sudo tar -czf simple-ldapd-logs-backup-$(date +%Y%m%d).tar.gz /var/log/simple-ldapd/
```

### Service Recovery

```bash
# Stop service
sudo systemctl stop simple-ldapd

# Restore configuration
sudo tar -xzf simple-ldapd-config-backup-YYYYMMDD.tar.gz -C /

# Start service
sudo systemctl start simple-ldapd
```

## Updates

### Service Update Process

1. **Stop service:**
   ```bash
   sudo systemctl stop simple-ldapd
   ```

2. **Backup current version:**
   ```bash
   sudo cp /usr/bin/simple-ldapd /usr/bin/simple-ldapd.backup
   ```

3. **Install new version:**
   ```bash
   sudo cp simple-ldapd /usr/bin/
   sudo chmod +x /usr/bin/simple-ldapd
   ```

4. **Start service:**
   ```bash
   sudo systemctl start simple-ldapd
   ```

5. **Verify update:**
   ```bash
   sudo systemctl status simple-ldapd
   simple-ldapd --version
   ```
