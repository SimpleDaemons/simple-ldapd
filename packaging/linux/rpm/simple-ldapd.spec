Name:           simple-ldapd
Version:        ${VERSION}
Release:        1%{?dist}
Summary:        Lightweight LDAPv3 directory daemon
License:        Apache-2.0
URL:            https://github.com/SimpleDaemons/%{name}
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  openssl-devel
BuildRequires:  sqlite-devel
BuildRequires:  pkgconfig
Requires:       openssl-libs
Requires:       sqlite-libs

%description
simple-ldapd is a lightweight LDAPv3 directory for SSO via LDAP bind.
It ships an OpenLDAP-style CLI. Default tool names match OpenLDAP
(ldapsearch, …); rebuild with -DLDAP_CLI_PREFIX=simple- if both must
share PATH.

%prep
%setup -q

%build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_TESTS=OFF
make %{?_smp_mflags}

%install
cd build
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_localstatedir}/lib/%{name}
mkdir -p %{buildroot}%{_localstatedir}/log/%{name}
mkdir -p %{buildroot}%{_sysconfdir}/%{name}/tls

%pre
if ! getent group simple-ldapd >/dev/null 2>&1; then
    groupadd -r simple-ldapd
fi
if ! getent passwd simple-ldapd >/dev/null 2>&1; then
    useradd -r -M -d %{_localstatedir}/lib/%{name} -s /sbin/nologin \
        -g simple-ldapd -c "%{name} service user" simple-ldapd
fi

%post
if [ -f %{_sysconfdir}/%{name}/%{name}.conf ]; then
    chown root:simple-ldapd %{_sysconfdir}/%{name}/%{name}.conf
    chmod 0640 %{_sysconfdir}/%{name}/%{name}.conf
elif [ -f %{_sysconfdir}/%{name}/templates/production.conf ]; then
    cp %{_sysconfdir}/%{name}/templates/production.conf \
        %{_sysconfdir}/%{name}/%{name}.conf
    chown root:simple-ldapd %{_sysconfdir}/%{name}/%{name}.conf
    chmod 0640 %{_sysconfdir}/%{name}/%{name}.conf
fi
chown root:simple-ldapd %{_sysconfdir}/%{name} %{_sysconfdir}/%{name}/tls 2>/dev/null || true
chmod 0750 %{_sysconfdir}/%{name} %{_sysconfdir}/%{name}/tls 2>/dev/null || true
chown simple-ldapd:simple-ldapd %{_localstatedir}/lib/%{name} %{_localstatedir}/log/%{name}
chmod 0750 %{_localstatedir}/lib/%{name} %{_localstatedir}/log/%{name}
if command -v systemd-tmpfiles >/dev/null 2>&1; then
    systemd-tmpfiles --create %{_prefix}/lib/tmpfiles.d/%{name}.conf >/dev/null 2>&1 || true
fi
if command -v systemctl >/dev/null 2>&1; then
    systemctl daemon-reload >/dev/null 2>&1 || true
fi

%preun
if [ "$1" -eq 0 ] && command -v systemctl >/dev/null 2>&1; then
    systemctl stop %{name} >/dev/null 2>&1 || true
    systemctl disable %{name} >/dev/null 2>&1 || true
fi

%postun
if command -v systemctl >/dev/null 2>&1; then
    systemctl daemon-reload >/dev/null 2>&1 || true
fi

%files
%license LICENSE
%doc README.md
%{_bindir}/%{name}
%{_bindir}/ldapsearch
%{_bindir}/ldapadd
%{_bindir}/ldapmodify
%{_bindir}/ldapdelete
%{_bindir}/ldappasswd
%{_bindir}/ldapcompare
%{_bindir}/ldapwhoami
%{_includedir}/simple-ldapd/
%{_unitdir}/%{name}.service
%{_prefix}/lib/sysusers.d/%{name}.conf
%{_prefix}/lib/tmpfiles.d/%{name}.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/%{name}
%dir %attr(0750,root,simple-ldapd) %{_sysconfdir}/%{name}
%dir %attr(0750,root,simple-ldapd) %{_sysconfdir}/%{name}/tls
%dir %attr(0750,simple-ldapd,simple-ldapd) %{_localstatedir}/lib/%{name}
%dir %attr(0750,simple-ldapd,simple-ldapd) %{_localstatedir}/log/%{name}
%{_sysconfdir}/%{name}/templates/
%{_sysconfdir}/%{name}/schemas/
%{_sysconfdir}/%{name}/examples/
%ghost %config(noreplace) %{_sysconfdir}/%{name}/%{name}.conf

%changelog
* Mon Aug 24 2026 SimpleDaemons <info@simpledaemons.com> - ${VERSION}-1
- Align packaging with production paths and systemd unit
