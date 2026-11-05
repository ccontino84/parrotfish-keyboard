# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

Name:       parrotfish-keyboard
Version:    0.1.0~alpha119
Release:    1
Summary:    Better suggestions and autocorrect for the Sailfish OS keyboard
License:    LGPL-2.1-or-later AND BSD-3-Clause AND Apache-2.0 AND CC0-1.0 AND LicenseRef-CC-BY-2.0-FR
URL:        https://github.com/ccontino84/parrotfish-keyboard
Source0:    %{name}-%{version}.tar.bz2
# Test versions (with a ~ suffix, as 0.1.0~alpha117) also install the
# developer tools: pf-query and the benchmarks.
%global devtools %(echo '%{version}' | grep -q '~' && echo ON || echo OFF)
BuildRequires:  cmake
BuildRequires:  python3-base
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(dconf)
Requires:   jolla-keyboard
Requires:   qt5-qtdeclarative

%description
Word suggestions, autocorrect with undo, learning and next-word prediction
for the Sailfish OS keyboard, in several languages at once: the typing
language follows the text. Adds four layouts (QWERTY, QWERTZ, AZERTY,
QWERTY with å ö ä) to the stock keyboard, with a prediction handler and a
settings section in Text input; no system files are modified, and Jolla's
own layouts are left as they are.

%prep
%autosetup -n %{name}-%{version}

%build
cmake -S . -B build-rpm \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LIBDIR=%{_lib} \
    -DPARROTFISH_QML_DIR=%{_libdir}/qt5/qml \
    -DPARROTFISH_DEVTOOLS=%{devtools} \
    -DPARROTFISH_VERSION=%{version}
cmake --build build-rpm -- %{?_smp_mflags}

%install
DESTDIR=%{buildroot} cmake --install build-rpm

# Restart the keyboard (a systemd user service) so that it loads the new
# files: systemctl-user (Sailfish's systemd package) runs systemctl --user for
# the logged-in user. %posttrans: after the whole upgrade, old files removed.
%posttrans
systemctl-user try-restart maliit-server.service || :

%postun
if [ $1 -eq 0 ]; then
    systemctl-user try-restart maliit-server.service || :
fi

%files
%license LICENSES/* THIRD-PARTY.md data/aosp/NOTICE
%{_libdir}/qt5/qml/Parrotfish
%{_datadir}/maliit/plugins/com/jolla/handlers/ParrotfishHandler.qml
%{_datadir}/maliit/plugins/com/jolla/handlers/parrotfish-keyboard.conf
%{_datadir}/maliit/plugins/com/jolla/ParrotfishHandler.qml
%{_datadir}/maliit/plugins/com/jolla/layouts/parrotfish*.qml
%{_datadir}/maliit/plugins/com/jolla/layouts/layouts_parrotfish.conf
%{_datadir}/maliit/plugins/com/jolla/layouts/parrotfish
%{_datadir}/%{name}
%if "%{devtools}" == "ON"
%{_libexecdir}/%{name}
%endif
