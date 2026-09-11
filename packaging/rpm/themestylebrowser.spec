# Version is injected by packaging/rpm/Makefile via `zfr version`.
# RPM Version cannot contain '-'; use `zfr version -r` (hyphens → '_').
# srcversion is the unsanitized Meson/git version and names the tarball.
%{!?version:%global version 0.0.0}
%{!?srcversion:%global srcversion %{version}}

Name:           themestylebrowser
Version:        %{version}
Release:        1%{?dist}
Summary:        Theme style icon library browser

License:        AGPL-3.0-or-later
URL:            https://github.com/example/themestylebrowser
Packager:       Lenik <themestylebrowser@bodz.net>
Source0:        %{name}-%{srcversion}.tar.xz

BuildRequires:  meson
BuildRequires:  ninja-build
BuildRequires:  pkg-config
BuildRequires:  libbas-c-dev
BuildRequires:  libbas-cpp-dev
BuildRequires:  libbas-ui-dev
BuildRequires:  libwxgtk3.2-dev
BuildRequires:  asciidoctor
Requires:       libbas-c1
Requires:       libbas-cpp1
Requires:       libbas-ui1
Requires:       libwxgtk3.2-1t64

%description
Theme Style Browser (tsb) browses icon libraries organized by
theme/style and package. The library root contains a .themestyles
file listing theme/style directories (e.g. flex/regular, pixel).
.
The UI shows a package tree on the left and a file list on the right
with columns per themestyle. Supports sorting by name, count, or size,
image thumbnails, and property dialogs with path/code snippet copy.

%prep
%setup -q -n %{name}-%{srcversion}

%build
meson setup build \
    --prefix=%{_prefix} \
    --bindir=%{_bindir} \
    --datadir=%{_datadir} \
    --mandir=%{_mandir} \
    --sysconfdir=%{_sysconfdir} \
    --localstatedir=%{_localstatedir} \
    --buildtype=plain
meson compile -C build

%install
meson install -C build --destdir=%{buildroot}

%files
%{_bindir}/themestylebrowser
%{_datadir}/bash-completion/completions/themestylebrowser
%{_mandir}/man1/themestylebrowser.1*
%{_datadir}/locale/*/LC_MESSAGES/themestylebrowser.mo
%{_mandir}/*/man1/themestylebrowser.1*
%{_datadir}/doc/themestylebrowser/
%changelog
* Thu Aug 20 2026 Lenik <themestylebrowser@bodz.net>
- Align spec with debian/control (Meson, AGPL-3.0-or-later).
- Version comes from `zfr version`, the same method meson.build uses.
