# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

Name:       parrotfish-keyboard
Version:    1.0.0
Release:    1
Summary:    Better suggestions and autocorrect for the Sailfish OS keyboard
License:    LGPL-2.1-or-later AND BSD-3-Clause AND Apache-2.0 AND CC0-1.0 AND LicenseRef-CC-BY-2.0-FR
URL:        https://github.com/ccontino84/parrotfish-keyboard
Source0:    %{name}-%{version}.tar.bz2
# Test versions (with a ~ suffix, as 0.1.0~alpha117) also install the
# developer tools: pf-query and the benchmarks.
%global devtools %(echo '%{version}' | grep -q '~' && echo ON || echo OFF)
# The dictionaries' format (FormatVersion in engine/include/parrotfish/
# dictformat.h; ctest `spec` checks it): a language package works with any
# keyboard of the same format, so a keyboard update needs no new
# dictionaries unless the format changed.
%global dictformat 5
BuildRequires:  cmake
BuildRequires:  python3-base
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(dconf)
# lrelease, for the settings' translations
BuildRequires:  qt5-qttools-linguist
Requires:   jolla-keyboard
Requires:   qt5-qtdeclarative
# No Requires on a language package: each would wait for the other when
# installed one file at a time (from the Files app). Without one, the
# keyboard types without suggestions, and the settings say so.
Provides:   parrotfish-dictionary-format = %{dictformat}

# Each %description ends with the metadata for SailfishOS:Chum's app
# (https://github.com/sailfishos-chum/main/blob/main/Metadata.md): it must
# stay the description's last paragraph, with no empty or comment lines in
# it. Other builds leave it out (_chum is defined only on Chum's build
# service).
%description
Word suggestions, autocorrect with undo, learning and next-word prediction
for the Sailfish OS keyboard, in several languages at once: the typing
language follows the text. Adds four layouts (QWERTY, QWERTZ, AZERTY,
QWERTY with å ö ä) to the stock keyboard, with a prediction handler and a
settings section in Text input; no system files are modified, and Jolla's
own layouts are left as they are.

The dictionaries come in language packages: install
parrotfish-languages-latin, parrotfish-languages-latin-experimental or
both. Each one installs the keyboard too.

%if 0%{?_chum}
Title: Parrotfish Keyboard
Type: inputmethod
DeveloperName: ccontino84
Categories:
 - Utility
Custom:
  Repo: https://github.com/ccontino84/parrotfish-keyboard
Links:
  Homepage: https://github.com/ccontino84/parrotfish-keyboard
  Bugtracker: https://github.com/ccontino84/parrotfish-keyboard/issues
%endif

%package -n parrotfish-languages-latin
Summary:    Languages for Parrotfish Keyboard: Czech, Dutch, English, French, German, Italian, Polish, Portuguese, Slovenian, Spanish
# The Slovenian dictionary comes from the Sloleks lexicon and the Kres
# n-grams (CC BY-SA 4.0).
License:    Apache-2.0 AND CC0-1.0 AND LicenseRef-CC-BY-2.0-FR AND CC-BY-SA-4.0
BuildArch:  noarch
Requires:   %{name}
Requires:   parrotfish-dictionary-format = %{dictformat}
Provides:   parrotfish-dictionaries
# Its name in the test builds (0.1.0~alpha123 to the 1.0.0 drafts).
Obsoletes:  parrotfish-latin-languages < 1.0.1

%description -n parrotfish-languages-latin
Dictionaries and word-pair data for Parrotfish Keyboard, for languages
written in the Latin script: Czech, Dutch, English, French, German,
Italian, Polish, Portuguese (Brazil and Portugal), Slovenian and Spanish.
Installs Parrotfish Keyboard too.

%if 0%{?_chum}
Title: Parrotfish Latin
Type: addon
DeveloperName: ccontino84
Categories:
 - Utility
Custom:
  Repo: https://github.com/ccontino84/parrotfish-keyboard
Links:
  Homepage: https://github.com/ccontino84/parrotfish-keyboard
  Bugtracker: https://github.com/ccontino84/parrotfish-keyboard/issues
%endif

%package -n parrotfish-languages-latin-experimental
Summary:    Experimental languages for Parrotfish Keyboard: Croatian, Danish, Finnish, Norwegian, Romanian, Swedish
License:    Apache-2.0 AND CC0-1.0 AND LicenseRef-CC-BY-2.0-FR
BuildArch:  noarch
Requires:   %{name}
Requires:   parrotfish-dictionary-format = %{dictformat}
Provides:   parrotfish-dictionaries
# Its names in the test builds.
Obsoletes:  parrotfish-keyboard-experimental-languages < 0.1.0~alpha123
Obsoletes:  parrotfish-latin-languages-experimental < 1.0.1

%description -n parrotfish-languages-latin-experimental
Dictionaries for the Latin-script languages Parrotfish Keyboard supports
less well, for lack of public text to build and test them: Croatian,
Danish, Finnish, Norwegian Bokmål, Romanian and Swedish. Installs
Parrotfish Keyboard too.

%if 0%{?_chum}
Title: Parrotfish Latin (exp.)
Type: addon
DeveloperName: ccontino84
Categories:
 - Utility
Custom:
  Repo: https://github.com/ccontino84/parrotfish-keyboard
Links:
  Homepage: https://github.com/ccontino84/parrotfish-keyboard
  Bugtracker: https://github.com/ccontino84/parrotfish-keyboard/issues
%endif

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
# The dictionaries go to the language packages.
python3 -I tools/split-languages.py %{buildroot}%{_datadir}/%{name} %{_datadir}/%{name}

# Restart the keyboard (a systemd user service) so that it loads the new
# files: systemctl-user (Sailfish's systemd package) runs systemctl --user for
# the logged-in user. %posttrans: after the whole upgrade, old files removed.
%posttrans
systemctl-user try-restart maliit-server.service || :

%postun
if [ $1 -eq 0 ]; then
    systemctl-user try-restart maliit-server.service || :
fi

%posttrans -n parrotfish-languages-latin
systemctl-user try-restart maliit-server.service || :

%postun -n parrotfish-languages-latin
if [ $1 -eq 0 ]; then
    systemctl-user try-restart maliit-server.service || :
fi

%posttrans -n parrotfish-languages-latin-experimental
systemctl-user try-restart maliit-server.service || :

%postun -n parrotfish-languages-latin-experimental
if [ $1 -eq 0 ]; then
    systemctl-user try-restart maliit-server.service || :
fi

%files -f main-excludes.files
%license LICENSES/* THIRD-PARTY.md data/aosp/NOTICE
%{_libdir}/qt5/qml/Parrotfish
%{_datadir}/maliit/plugins/com/jolla/handlers/ParrotfishHandler.qml
%{_datadir}/maliit/plugins/com/jolla/handlers/parrotfish-keyboard.conf
%{_datadir}/maliit/plugins/com/jolla/ParrotfishHandler.qml
%{_datadir}/maliit/plugins/com/jolla/layouts/parrotfish*.qml
%{_datadir}/maliit/plugins/com/jolla/layouts/layouts_parrotfish.conf
%{_datadir}/maliit/plugins/com/jolla/layouts/parrotfish
%{_bindir}/parrotfish
%{_datadir}/translations/parrotfish-keyboard-*.qm
%{_datadir}/%{name}
%if "%{devtools}" == "ON"
%{_libexecdir}/%{name}
%endif

%files -n parrotfish-languages-latin -f latin.files
%license LICENSES/* THIRD-PARTY.md data/aosp/NOTICE

%files -n parrotfish-languages-latin-experimental -f latin-experimental.files
%license LICENSES/* THIRD-PARTY.md data/aosp/NOTICE
