# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

# QML module Parrotfish.Keyboard, loaded by the handler inside maliit-server.
# Qt 5.6 on Sailfish OS: keep to its API.
find_package(Qt5 REQUIRED COMPONENTS Core Qml)
set(CMAKE_AUTOMOC ON)

add_library(parrotfishkeyboard MODULE
    src/plugin.cpp
    src/predictionengine.cpp
)
target_link_libraries(parrotfishkeyboard PRIVATE parrotfish-engine Qt5::Core Qt5::Qml)
target_compile_definitions(parrotfishkeyboard PRIVATE
    PARROTFISH_DATA_DIR="${PARROTFISH_DATA_DIR}"
    QT_NO_CAST_FROM_ASCII)
target_compile_options(parrotfishkeyboard PRIVATE -Wall -Wextra)

install(TARGETS parrotfishkeyboard DESTINATION ${PARROTFISH_QML_DIR}/Parrotfish/Keyboard)
install(FILES qmldir DESTINATION ${PARROTFISH_QML_DIR}/Parrotfish/Keyboard)
