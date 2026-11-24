#!/bin/bash
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# The settings pages on a PC (ctest: settings): every page loaded with
# stand-ins for Silica (stubs/ over tests/qml/stubs) and the real plugin,
# checked for what they show for a few dconf states (tst_settings.qml).
# The pages' dconf is the stub's; KeyboardInfo's the real one, in a private
# D-Bus session. Any QML warning fails the run. Silica's real behaviour is
# not tested.
# usage: tests/settings/run.sh BUILD_DIR
set -eo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(dirname "$(dirname "$HERE")")
BUILD=$(cd "${1:?usage: run.sh BUILD_DIR}" && pwd)
RUNNER=${QMLTESTRUNNER:-$(command -v qmltestrunner-qt5 || command -v qmltestrunner)}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/imports/Parrotfish/Keyboard" "$TMP/home/.local/share/parrotfish-keyboard"
cp -r "$SRC/tests/qml/stubs/." "$TMP/imports/"
# Ours over the handler test's; new Silica types join its qmldir.
for f in "$HERE"/stubs/Sailfish/Silica/*.qml; do
    n=$(basename "$f" .qml)
    if ! grep -q "^\(singleton \)\?$n " "$TMP/imports/Sailfish/Silica/qmldir"; then
        if head -1 "$f" | grep -q Singleton; then
            echo "singleton $n 1.0 $n.qml" >> "$TMP/imports/Sailfish/Silica/qmldir"
        else
            echo "$n 1.0 $n.qml" >> "$TMP/imports/Sailfish/Silica/qmldir"
        fi
    fi
done
cp -r "$HERE/stubs/." "$TMP/imports/"
cp "$SRC/plugin/qmldir" "$BUILD/plugin/libparrotfishkeyboard.so" "$TMP/imports/Parrotfish/Keyboard/"
cp "$SRC"/plugin/emoji/*.js "$TMP/imports/Parrotfish/Keyboard/"
# The build's dictionaries but fr, uk and el (to download), and a
# downloaded de that is not this release's (an update).
mkdir "$TMP/data" "$TMP/home/.local/share/parrotfish-keyboard/dictionaries"
for f in "$BUILD"/data/*; do
    case $(basename "$f") in fr.pfd|uk.pfd|el.pfd) ;; *) ln -s "$f" "$TMP/data/" ;; esac
done
echo "not this release's" > "$TMP/home/.local/share/parrotfish-keyboard/dictionaries/de.pfd"
sha256sum "$TMP/home/.local/share/parrotfish-keyboard/dictionaries/de.pfd" | cut -c1-64 \
    > "$TMP/home/.local/share/parrotfish-keyboard/dictionaries/de.pfd.sha256"
cp -r "$SRC/settings" "$TMP/settings"
cp "$HERE/tst_settings.qml" "$TMP/settings/"
cd "$TMP/settings"
export HOME="$TMP/home" XDG_CONFIG_HOME="$TMP/home/.config" XDG_RUNTIME_DIR="$TMP/run"
mkdir -p "$XDG_RUNTIME_DIR" && chmod 700 "$XDG_RUNTIME_DIR"
export PARROTFISH_DATA_DIR="$TMP/data" QML2_IMPORT_PATH="$TMP/imports" RUNNER TMP
export QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software QT_FORCE_STDERR_LOGGING=1
dbus-run-session -- bash -c '
    set -eo pipefail
    dconf write /apps/parrotfish-keyboard/autocorrect "'"'"'normal'"'"'"
    dconf write /apps/parrotfish-keyboard/languages "'"'"'en it'"'"'"
    dconf write /apps/parrotfish-keyboard/number-row false
    dconf write /apps/parrotfish-keyboard/sticky-key "'"'"'@'"'"'"
    dconf write /apps/parrotfish-keyboard/key-tuning "'"'"'dwell=300'"'"'"
    timeout 120 "$RUNNER" -input tst_settings.qml 2>&1 | tee "$TMP/log"
    left=$(dconf dump /apps/parrotfish-keyboard/)
    if [ -n "$left" ]; then echo "FAIL!  : keys left after Reset:"; echo "$left"; exit 1; fi
    echo "PASS   : nothing left under /apps/parrotfish-keyboard/ after Reset"'
# QML warnings (a TypeError in a binding, a missing property) fail it too;
# not Qt 5.15's about the Connections syntax Qt 5.6 needs.
if grep -E '\.qml:[0-9]+(:[0-9]+)?: ' "$TMP/log" | grep -v 'Implicitly defined onFoo'; then
    echo "FAIL!  : QML warnings above"
    exit 1
fi
