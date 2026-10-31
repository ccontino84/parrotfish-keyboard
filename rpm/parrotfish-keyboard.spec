# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

Name:       parrotfish-keyboard
Version:    0.1.0~alpha70
Release:    1
Summary:    Better suggestions and autocorrect for the Sailfish OS keyboard
License:    LicenseRef-TBD AND Apache-2.0 AND CC0-1.0 AND LicenseRef-CC-BY-2.0-FR
URL:        https://github.com/ccontino84/parrotfish-keyboard
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  cmake
BuildRequires:  python3-base
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(sqlite3)
Requires:   jolla-keyboard
Requires:   qt5-qtdeclarative

%description
A keyboard layout for the Sailfish OS keyboard (QWERTY or QWERTZ) with
word suggestions and autocorrect in English, Italian and German at once:
the typing language follows the text. Plugs into the stock keyboard as a
layout plus a prediction handler; no system files are modified, and
Jolla's own layouts are left as they are.

%prep
%autosetup -n %{name}-%{version}

%build
cmake -S . -B build-rpm \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LIBDIR=%{_lib} \
    -DPARROTFISH_QML_DIR=%{_libdir}/qt5/qml \
    -DPARROTFISH_DEVTOOLS=ON \
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
%{_datadir}/maliit/plugins/com/jolla/layouts/parrotfish.qml
%{_datadir}/maliit/plugins/com/jolla/layouts/layouts_parrotfish.conf
%{_datadir}/maliit/plugins/com/jolla/layouts/parrotfish
%{_datadir}/%{name}
%{_libexecdir}/%{name}
