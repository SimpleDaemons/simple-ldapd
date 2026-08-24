# Packaging Templates

This directory contains templates for creating installers and packages for simple-ldapd across different platforms.

CPack in the top-level `CMakeLists.txt` is the supported package path (`make package`). These files are the maintainer scripts, spec, and installer sources that CPack and manual packaging use.

## Installed layout (must match production templates)

| Platform | Binary | Config | Data | Logs |
|----------|--------|--------|------|------|
| Linux (`CMAKE_INSTALL_PREFIX=/usr`) | `/usr/bin/simple-ldapd` | `/etc/simple-ldapd/simple-ldapd.conf` | `/var/lib/simple-ldapd` | `/var/log/simple-ldapd` |
| macOS | `/usr/local/bin/simple-ldapd` | `/etc/simple-ldapd/simple-ldapd.conf` | `/var/lib/simple-ldapd` | `/var/log/simple-ldapd` |
| Windows | `%PROGRAMFILES%\simple-ldapd\simple-ldapd.exe` | `%PROGRAMDATA%\simple-ldapd\simple-ldapd.conf` | `%PROGRAMDATA%\simple-ldapd` | `%PROGRAMDATA%\simple-ldapd\logs` |

Linux units start `--config` then `--foreground` at those paths so the flag wins over `foreground = false` in the production templates. Packages install templates, examples, and schemas under `/etc/simple-ldapd/`, documentation under `/usr/share/doc/simple-ldapd/`, and create the data/log directories and the `simple-ldapd` service user; they do not enable or start the daemon (TLS and `root_password` are still unset).

Default CLI names match OpenLDAP (`ldapsearch`, …). Rebuild with `-DLDAP_CLI_PREFIX=simple-` or a private `-DCMAKE_INSTALL_PREFIX` if both must share `PATH`.

## Directory Structure

```
packaging/
├── macos/
│   ├── pkg/                    # macOS PackageMaker (PKG) installer
│   │   ├── Distribution.xml     # Package distribution configuration
│   │   ├── PackageInfo.xml      # Package metadata
│   │   └── scripts/
│   │       └── postinstall      # Post-installation script
│   └── dmg/                     # macOS Disk Image (DMG)
│       └── create-dmg.sh        # DMG creation script
├── windows/
│   ├── nsis/                    # NSIS installer
│   │   └── installer.nsi        # NSIS installer script
│   └── msi/                     # Windows Installer (MSI)
│       └── installer.wxs        # WiX installer script
├── linux/
│   ├── deb/                     # Debian/Ubuntu packages
│   │   ├── control              # Debian control file
│   │   └── postinst             # User, dirs, ownership (non-interactive)
│   └── rpm/                     # Red Hat/CentOS packages
│       └── simple-ldapd.spec    # RPM spec (user, dirs, systemd unit)
├── assets/
│   ├── icons/                   # Installer icons and graphics
│   │   ├── simple-ldapd.ico   # Windows icon
│   │   ├── simple-ldapd.icns  # macOS icon
│   │   ├── header.bmp           # NSIS header image
│   │   ├── wizard.bmp           # NSIS wizard image
│   │   ├── background.png       # PKG background
│   │   └── dmg-background.png   # DMG background
│   ├── welcome.html             # Welcome page for PKG
│   ├── readme.html              # Read me page
│   └── conclusion.html          # Installation complete page
└── licenses/
    ├── LICENSE.txt              # Plain text license
    └── LICENSE.rtf               # Rich text license for Windows
```

## Features

### License Acceptance
- **macOS PKG**: License displayed and must be accepted
- **Windows NSIS/MSI**: License page with acceptance required
- **Linux DEB/RPM**: Apache-2.0 is shipped with the package (`%license` / copyright). Maintainer scripts are non-interactive so unattended installs work.

### Custom Icons and Graphics
- Windows: `.ico` files for application and installer
- macOS: `.icns` files for application, PNG for backgrounds
- Custom header/wizard images for NSIS installers
- DMG background images

### Platform-Specific Features

#### macOS PKG
- Modern installer with welcome/readme/conclusion pages
- License acceptance
- Post-installation scripts
- Service user creation
- LaunchDaemon integration

#### macOS DMG
- Custom background image
- Applications symlink
- License and README included
- Compressed format (UDZO)

#### Windows NSIS
- Modern UI with custom graphics
- License acceptance page
- Component selection
- Start Menu shortcuts
- Windows Service installation
- Uninstaller included

#### Windows MSI
- WiX-based installer
- License acceptance
- Service installation
- Registry entries
- Upgrade support

#### Linux DEB
- Non-interactive postinst (service user, `/var/lib` and `/var/log`, config mode `640`)
- Systemd unit, sysusers, tmpfiles, logrotate
- Configuration file installation (`templates/production.conf` → `simple-ldapd.conf` if missing)

#### Linux RPM
- Non-interactive `%pre`/`%post` (service user, directories)
- Systemd integration
- Proper file placement including empty data/log dirs

## Usage

### Building Packages

#### macOS PKG
```bash
# Build the project first
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# Create package
cpack -G PackageMaker
```

#### macOS DMG
```bash
# After creating PKG
cd packaging/macos/dmg
./create-dmg.sh 1.0.0
```

#### Windows NSIS
```bash
# Build NSIS installer
makensis packaging/windows/nsis/installer.nsi
```

#### Windows MSI
```bash
# Build with WiX
candle packaging/windows/msi/installer.wxs
light installer.wixobj
```

#### Linux DEB
```bash
# Build Debian package
dpkg-buildpackage -us -uc
```

#### Linux RPM
```bash
# Build RPM package
rpmbuild -ba packaging/linux/rpm/simple-ldapd.spec
```

## Template Variables

Replace these placeholders in templates:
- `simple-ldapd` - Project name (e.g., simple-dhcpd)
- `simple-ldapd` - Service user name (e.g., dhcpdev)
- `{PROJECT_GROUP}` - Service group name
- `LDAP` - Protocol name (e.g., DHCP, NTP)
- `${VERSION}` - Version number (set during build)

## Icon Requirements

### Windows
- **Application Icon**: 256x256, `.ico` format
- **Header Image**: 150x57, `.bmp` format
- **Wizard Image**: 164x314, `.bmp` format

### macOS
- **Application Icon**: 512x512, `.icns` format
- **Background**: 620x418, `.png` format
- **DMG Background**: 658x498, `.png` format

## License Files

- **LICENSE.txt**: Plain text license (for all platforms)
- **LICENSE.rtf**: Rich text format license (for Windows MSI)

## Notes

- All scripts must be executable (`chmod +x`)
- License files must be present in `packaging/licenses/`
- Icon files must be present in `packaging/assets/icons/`
- Update GUIDs in MSI/WiX templates with unique values
- Test installers on clean systems before distribution

