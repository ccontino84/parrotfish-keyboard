#!/bin/bash
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# The QML tests on a PC (ctest: qml): our handler with the engine plugin and
# the dictionaries of BUILD_DIR, Sailfish OS's modules replaced by the
# stand-ins in stubs/ (Jolla's InputHandler, maliit, Silica, dconf). Keys
# are tapped by calling handleKeyClick(); MInputMethodQuick.committedText
# collects what reached the field. A home of its own: the debug file on
# (slips.log), a fresh learning.db. Needs qmltestrunner (Qt 5); Qt Quick
# draws in software, so no OpenGL is needed (CI containers have none).
# usage: tests/qml/run.sh BUILD_DIR
set -eo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(dirname "$(dirname "$HERE")")
BUILD=$(cd "${1:?usage: run.sh BUILD_DIR}" && pwd)
RUNNER=${QMLTESTRUNNER:-$(command -v qmltestrunner-qt5 || command -v qmltestrunner)}

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/imports/Parrotfish/Keyboard" "$TMP/handlers" "$TMP/home/.local/share/parrotfish-keyboard"
cp -r "$HERE/stubs/." "$TMP/imports/"
cp "$SRC/plugin/qmldir" "$BUILD/plugin/libparrotfishkeyboard.so" "$TMP/imports/Parrotfish/Keyboard/"
ln -s "$SRC/handler/ParrotfishHandler.qml" "$TMP/handlers/ParrotfishHandler.qml"
cp "$SRC/plugin/emoji/emojiutil.js" "$SRC/plugin/emoji/emoticons.js" "$TMP/imports/Parrotfish/Keyboard/"
touch "$TMP/home/.local/share/parrotfish-keyboard/debug"
sed -e "s|HOMEPATH|$TMP/home|" "$HERE/tst_handler.qml" > "$TMP/tst_handler.qml"
cd "$TMP"
HOME="$TMP/home" QML_XHR_ALLOW_FILE_READ=1 PARROTFISH_DATA_DIR="$BUILD/data" \
    QML2_IMPORT_PATH="$TMP/imports" QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software QT_FORCE_STDERR_LOGGING=1 \
    timeout 120 "$RUNNER" -input tst_handler.qml
