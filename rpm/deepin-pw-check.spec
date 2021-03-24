# Run tests in check section
# disable for bootstrapping
%bcond_with check
%global with_debug 1
%if 0%{?with_debug}
%global debug_package   %{nil}
%endif
Name:           deepin-pw-check
Version:        5.0.13
Release:        1
Summary:        Used to check password and manager the configuration for password.
License:        GPLv3
URL:            %{gourl}
Source0:        %{name}-%{version}.orig.tar.xz
BuildRequires:  compiler(go-compiler)
BuildRequires:  pam-devel
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(gdk-3.0)
BuildRequires:  gocode
BuildRequires:  deepin-gettext-tools
BuildRequires:  cracklib-devel
BuildRequires:  iniparser-devel
BuildRequires:  golang-github-linuxdeepin-go-dbus-factory-devel
BuildRequires:  go-lib-devel
BuildRequires:  go-gir-generator

Requires:		polkit
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
patch -p1 < rpm/0001-fix-for-UonioTech.patch

%build
BUILDID="0x$(head -c20 /dev/urandom|od -An -tx1|tr -d ' \n')"
export GOPATH=/usr/share/gocode
%make_build GO_BUILD_FLAGS=-trimpath GOBUILD="go build -compiler gc -ldflags \"-B $BUILDID\""

%install
export GOPATH=%{_datadir}/gocode
export PKG_FILE_DIR=%{_libdir}/pkgconfig
%make_install PKG_FILE_DIR=%{_libdir}/pkgconfig LIBDIR=lib64 PAM_MODULE_DIR=%{_libdir}/security GOBUILD="go build -compiler gc -ldflags \"-B $BUILDID\""
%find_lang deepin-pw-check


%files -f deepin-pw-check.lang
%doc README.md
%license
%{_bindir}/pwd-conf-update
%{_prefix}/lib/deepin-pw-check/deepin-pw-check
%{_libdir}/libdeepin_pw_check.so.*
%{_libdir}/security/pam_deepin_pw_check.so
%{_datadir}/dbus-1/system-services/*.service
%{_datadir}/dbus-1/system.d/*.conf
%{_datadir}/polkit-1/actions/*.policy

%files devel
%{_libdir}/libdeepin_pw_check.a
%{_libdir}/libdeepin_pw_check.so
%{_libdir}/pkgconfig/libdeepin_pw_check.pc
%{_includedir}/deepin_pw_check.h

%changelog
* Wed Mar 12 2021 uoser <uoser@uniontech.com> - 5.0.13-1
- Update to 5.0.13