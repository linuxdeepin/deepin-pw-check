# Run tests in check section
# disable for bootstrapping
%bcond_with check
%global with_debug 1
%if 0%{?with_debug}
%global debug_package   %{nil}
%endif
Name:           deepin-pw-check
Version:        5.1.4
Release:        1%{?dist}
Summary:        Used to check password and manager the configuration for password.
License:        GPLv3
URL:            %{gourl}
Source0:        %{name}-%{version}.tar.gz

Patch1000:	0001-fix-for-UonioTech.patch
Patch1001:	0001-Mangle-Suit-Cracklib2.9.6.patch

BuildRequires:  compiler(go-compiler)
BuildRequires:  pam-devel
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(gdk-3.0)
BuildRequires:  deepin-gettext-tools
BuildRequires:  golang-github-fsnotify-fsnotify-devel
#BuildRequires:  golang-github-godbus-dbus-devel
BuildRequires:  golang-dbus

BuildRequires:  cracklib-devel 
BuildRequires:  iniparser-devel
BuildRequires:  golang-github-linuxdeepin-go-dbus-factory-devel
BuildRequires:  go-lib-devel
BuildRequires:  go-gir-generator
BuildRequires:  golang-github-stretchr-testify-devel

Requires:	polkit
%description
In order to unify the authentication interface,
this interface is designed to adapt to fingerprint, face and other authentication methods.

%package devel
Summary: Header files and libraries used to build deepin-pw-check
Requires: %{name} = %{version}-%{release}
Requires: cracklib-devel iniparser-devel

%description devel
In order to unify the authentication interface,
this interface is designed to adapt to fingerprint, face and other authentication methods.

%prep
%setup -q
%patch1000  -p1
%patch1001  -p1

%build
BUILDID="0x$(head -c20 /dev/urandom|od -An -tx1|tr -d ' \n')"
export GO111MODULE=off
export GOPATH=/usr/share/gocode
%make_build GO_BUILD_FLAGS=-trimpath GOBUILD="go build -compiler gc -ldflags \"-B $BUILDID\""

%post
pwd-conf-update
sed -i "s/pam_pwquality.so/pam_deepin_pw_check.so/g" /etc/pam.d/system-auth
sed -i "s/pam_pwquality.so/pam_deepin_pw_check.so/g" /etc/pam.d/password-auth

%postun
if [ "$1" = "0" ] ; then
        sed -i "s/pam_deepin_pw_check.so/pam_pwquality.so/g" /etc/pam.d/system-auth
        sed -i "s/pam_deepin_pw_check.so/pam_pwquality.so/g" /etc/pam.d/password-auth
fi

%install
mkdir -p %{buildroot}/%{_sysconfdir}/deepin
export GOPATH=%{_datadir}/gocode
export PKG_FILE_DIR=%{_libdir}/pkgconfig
%make_install PKG_FILE_DIR=%{_libdir}/pkgconfig LIBDIR=lib64 PAM_MODULE_DIR=%{_libdir}/security GOBUILD="go build -compiler gc -ldflags \"-B $BUILDID\""
%find_lang deepin-pw-check

%ifarch i686
mv %{buildroot}/usr/lib64/libdeepin_pw_check.so* %{buildroot}/usr/lib/
%endif

%files -f deepin-pw-check.lang
%doc README.md
%license
%{_bindir}/pwd-conf-update
%{_prefix}/lib/deepin-pw-check/deepin-pw-check
%{_libdir}/libdeepin_pw_check.so.*
%ifarch i686
/usr/lib/security/pam_deepin_pw_check.so
%else
%{_libdir}/security/pam_deepin_pw_check.so
%endif
%{_datadir}/dbus-1/system-services/*.service
%{_datadir}/dbus-1/system.d/*.conf
%{_datadir}/polkit-1/actions/*.policy

%files devel
%ifarch i686
/usr/lib64/libdeepin_pw_check.a
/usr/lib/pkgconfig/libdeepin_pw_check.pc
%else
%{_libdir}/libdeepin_pw_check.a
%{_libdir}/pkgconfig/libdeepin_pw_check.pc
%endif
%{_libdir}/libdeepin_pw_check.so
%{_includedir}/deepin_pw_check.h

%changelog
* Mon Dec 13 2021 mahailiang <mahailiang@uniontech.com> - 5.1.4-1
- Update to 5.1.4

* Tue Oct 19 2021 Zhang taikun <zhangtaikun@uniontech.com> - 5.1.2-1
- Update to 5.1.2

* Fri Mar 12 2021 uoser <uoser@uniontech.com> - 5.0.13-1.01
- modify for i686

* Fri Mar 12 2021 uoser <uoser@uniontech.com> - 5.0.13-1
- Update to 5.0.13
