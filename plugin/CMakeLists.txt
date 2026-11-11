# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

# QML module Parrotfish.Keyboard, loaded by the handler inside maliit-server.
# Qt 5.6 on Sailfish OS: keep to its API.
find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(DCONF REQUIRED IMPORTED_TARGET dconf)
# Colour emoji (EmojiImageProvider): cairo draws COLRv1 and SVG glyphs.
pkg_check_modules(EMOJI REQUIRED IMPORTED_TARGET cairo freetype2 harfbuzz fontconfig)
set(CMAKE_AUTOMOC ON)

add_library(parrotfishkeyboard MODULE
    src/emojiimageprovider.cpp
    src/keyboardinfo.cpp
    src/languages.cpp
    src/learnedwords.cpp
    src/learningstore.cpp
    src/plugin.cpp
    src/predictionengine.cpp
    src/storewriter.cpp
)
target_link_libraries(parrotfishkeyboard PRIVATE parrotfish-engine Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick PkgConfig::SQLITE3 PkgConfig::DCONF PkgConfig::EMOJI Threads::Threads)
target_compile_definitions(parrotfishkeyboard PRIVATE
    PARROTFISH_DATA_DIR="${PARROTFISH_DATA_DIR}"
    PARROTFISH_VERSION="${PARROTFISH_VERSION}"
    PARROTFISH_TRANSLATIONS_DIR="${CMAKE_INSTALL_FULL_DATADIR}/translations"
    QT_NO_CAST_FROM_ASCII)
target_compile_options(parrotfishkeyboard PRIVATE -Wall -Wextra)

install(TARGETS parrotfishkeyboard DESTINATION ${PARROTFISH_QML_DIR}/Parrotfish/Keyboard)
install(FILES qmldir emoji/emojiutil.js emoji/emoticons.js DESTINATION ${PARROTFISH_QML_DIR}/Parrotfish/Keyboard)
