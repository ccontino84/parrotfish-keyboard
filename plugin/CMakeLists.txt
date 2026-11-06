# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

# QML module Parrotfish.Keyboard, loaded by the handler inside maliit-server.
# Qt 5.6 on Sailfish OS: keep to its API.
find_package(Qt5 REQUIRED COMPONENTS Core Qml)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(DCONF REQUIRED IMPORTED_TARGET dconf)
set(CMAKE_AUTOMOC ON)

add_library(parrotfishkeyboard MODULE
    src/keyboardinfo.cpp
    src/languages.cpp
    src/learnedwords.cpp
    src/learningstore.cpp
    src/plugin.cpp
    src/predictionengine.cpp
    src/storewriter.cpp
)
target_link_libraries(parrotfishkeyboard PRIVATE parrotfish-engine Qt5::Core Qt5::Qml PkgConfig::SQLITE3 PkgConfig::DCONF Threads::Threads)
target_compile_definitions(parrotfishkeyboard PRIVATE
    PARROTFISH_DATA_DIR="${PARROTFISH_DATA_DIR}"
    PARROTFISH_VERSION="${PARROTFISH_VERSION}"
    PARROTFISH_TRANSLATIONS_DIR="${CMAKE_INSTALL_FULL_DATADIR}/translations"
    QT_NO_CAST_FROM_ASCII)
target_compile_options(parrotfishkeyboard PRIVATE -Wall -Wextra)

install(TARGETS parrotfishkeyboard DESTINATION ${PARROTFISH_QML_DIR}/Parrotfish/Keyboard)
install(FILES qmldir DESTINATION ${PARROTFISH_QML_DIR}/Parrotfish/Keyboard)
