# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

Name:       parrotfish-keyboard
Version:    0.1.0~alpha18
Release:    1
Summary:    Better suggestions and autocorrect for the Sailfish OS keyboard
License:    LicenseRef-TBD AND Apache-2.0 AND CC0-1.0 AND LicenseRef-CC-BY-2.0-FR
URL:        https://github.com/ccontino84/parrotfish-keyboard
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  cmake
BuildRequires:  python3-base
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
Requires:   jolla-keyboard
Requires:   qt5-qtdeclarative

%description
Word suggestions for the English, Italian and German layouts of the
Sailfish OS keyboard, in all three languages at once. Plugs into the
stock keyboard as a prediction handler; no system files are modified.

%prep
%autosetup -n %{name}-%{version}

%build
cmake -S . -B build-rpm \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LIBDIR=%{_lib} \
    -DPARROTFISH_QML_DIR=%{_libdir}/qt5/qml \
    -DPARROTFISH_DEVTOOLS=ON
cmake --build build-rpm -- %{?_smp_mflags}

%install
DESTDIR=%{buildroot} cmake --install build-rpm

%post
echo "parrotfish-keyboard: restart the keyboard to load it: systemctl --user restart maliit-server"

%postun
if [ $1 -eq 0 ]; then
    echo "parrotfish-keyboard: restart the keyboard to unload it: systemctl --user restart maliit-server"
fi

%files
%license LICENSES/* THIRD-PARTY.md data/aosp/NOTICE
%{_libdir}/qt5/qml/Parrotfish
%{_datadir}/maliit/plugins/com/jolla/handlers/ParrotfishHandler.qml
%{_datadir}/maliit/plugins/com/jolla/handlers/parrotfish-keyboard.conf
%{_datadir}/maliit/plugins/com/jolla/layouts/parrotfish-qwerty.qml
%{_datadir}/maliit/plugins/com/jolla/layouts/parrotfish-qwertz.qml
%{_datadir}/maliit/plugins/com/jolla/layouts/layouts_parrotfish.conf
%{_datadir}/%{name}
%{_libexecdir}/%{name}
